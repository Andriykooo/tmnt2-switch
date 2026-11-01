// Recovers what each minified wasm import ("a", "b", ...) does by finding the
// emscripten import object in index.js and printing a snippet of each target function.
const fs = require('fs');

const js = fs.readFileSync(process.argv[2], 'utf8');
const out = process.argv[3];

// The import object is the only literal starting with {a:X,b:Y,c:Z,...
const start = js.search(/\{a:[\w$]+,b:[\w$]+,c:[\w$]+,d:[\w$]+/);
if (start < 0) throw new Error('import object not found');
let depth = 0, end = start;
for (; end < js.length; end++) {
  if (js[end] === '{') depth++;
  else if (js[end] === '}' && --depth === 0) break;
}
const obj = js.slice(start + 1, end);
const pairs = obj.split(',').map(p => p.split(':')).filter(p => p.length === 2);

function definition(name) {
  const esc = name.replace(/\$/g, '\\$');
  const patterns = [
    new RegExp(`function ${esc}\\(([^)]*)\\)\\{`),
    new RegExp(`[,;\\s]${esc}=\\(([^)]*)\\)=>`),
    new RegExp(`[,;\\s]${esc}=([\\w$]+)=>`),
    new RegExp(`[,;\\s]${esc}=function\\(([^)]*)\\)\\{`),
  ];
  for (const re of patterns) {
    const m = re.exec(js);
    if (m) return js.slice(m.index, m.index + 260).replace(/\s+/g, ' ');
  }
  const alias = new RegExp(`[,;\\s]${esc}=([\\w$.]+)[,;]`).exec(js);
  return alias ? `= ${alias[1]}` : '?';
}

const lines = pairs.map(([k, v]) => `${k}\t${v}\t${definition(v)}`);
fs.writeFileSync(out, lines.join('\n') + '\n');
console.log(`${pairs.length} imports mapped -> ${out}`);
