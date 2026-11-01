// Extracts every RT_ICON image from a PE file as standalone .ico files.
// usage: node extract-icons.js <exe> <outdir>
const fs = require('fs');
const path = require('path');

const [exe, outDir] = process.argv.slice(2);
const b = fs.readFileSync(exe);
const pe = b.readUInt32LE(0x3c);
const nsec = b.readUInt16LE(pe + 6);
const optSize = b.readUInt16LE(pe + 20);
const opt = pe + 24;
const dd = opt + (b.readUInt16LE(opt) === 0x20b ? 112 : 96);
const rsrcRva = b.readUInt32LE(dd + 2 * 8);
const secs = [];
for (let i = 0; i < nsec; i++) {
  const s = opt + optSize + i * 40;
  secs.push({ va: b.readUInt32LE(s + 12), vsz: b.readUInt32LE(s + 8), raw: b.readUInt32LE(s + 20) });
}
const off = (rva) => {
  const s = secs.find((s) => rva >= s.va && rva < s.va + Math.max(s.vsz, 1));
  return rva - s.va + s.raw;
};
const base = off(rsrcRva);
function entries(dirOff) {
  const n = b.readUInt16LE(dirOff + 12) + b.readUInt16LE(dirOff + 14);
  const out = [];
  for (let i = 0; i < n; i++) {
    const e = dirOff + 16 + i * 8;
    const id = b.readUInt32LE(e);
    const v = b.readUInt32LE(e + 4);
    out.push({ id, dir: !!(v & 0x80000000), off: base + (v & 0x7fffffff) });
  }
  return out;
}
fs.mkdirSync(outDir, { recursive: true });
const icons = entries(base).find((e) => e.id === 3);
if (!icons) throw new Error('no RT_ICON');
for (const name of entries(icons.off)) {
  for (const lang of entries(name.off)) {
    const rva = b.readUInt32LE(lang.off);
    const size = b.readUInt32LE(lang.off + 4);
    const data = b.subarray(off(rva), off(rva) + size);
    const isPng = data.readUInt32BE(0) === 0x89504e47;
    const w = isPng ? data.readUInt32BE(16) : data.readInt32LE(4);
    const hdr = Buffer.alloc(22);
    hdr.writeUInt16LE(1, 2);
    hdr.writeUInt16LE(1, 4);
    hdr.writeUInt8(w >= 256 ? 0 : w, 6);
    hdr.writeUInt8(w >= 256 ? 0 : w, 7);
    hdr.writeUInt16LE(1, 10);
    hdr.writeUInt16LE(isPng ? 32 : data.readUInt16LE(14), 12);
    hdr.writeUInt32LE(size, 14);
    hdr.writeUInt32LE(22, 18);
    const file = path.join(outDir, `icon${name.id}_${w}px_${isPng ? 32 : data.readUInt16LE(14)}bpp.ico`);
    fs.writeFileSync(file, Buffer.concat([hdr, data]));
    console.log(file);
  }
}
