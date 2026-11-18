// open-field: the Blackfin LDR loader-stream primitives, a faithful port of open-xy's MIT-licensed
// tooling/formats.py (which our tools/ldr_build.py uses). An LDR file is a run of 16-byte record headers,
// each optionally followed by its payload; the open-field base image is the stock os.ldr with two bytes
// edited, one record appended and the first header rewritten. No relocation, no payload checksum — the
// only checksum is one XOR byte per header.
//
// Portions derived from open-xy (https://github.com/etcetc000/open-xy), MIT License, (c) Open-XY
// contributors. 32-bit values are handled through DataView and `>>> 0`, never JS bitwise on sums, because
// Blackfin addresses exceed 2^31.

export function req(condition, message) {
  if (!condition) throw new Error(message);
}

const FLAG = 0xad000000;      // the top byte of every header's code word

// Build a 16-byte record header. The checksum byte (offset 2) is set so the XOR of all 16 bytes is 0.
export function header(flags, target, count, argument) {
  for (const n of [flags, target, count, argument]) req(n >= 0 && n <= 0xffffffff, "header field overflow");
  const out = new Uint8Array(16);
  const dv = new DataView(out.buffer);
  dv.setUint32(0, (FLAG + flags) >>> 0, true);
  dv.setUint32(4, target >>> 0, true);
  dv.setUint32(8, count >>> 0, true);
  dv.setUint32(12, argument >>> 0, true);
  let xor = 0;
  for (let i = 0; i < 16; i++) xor ^= out[i];   // out[2] is 0 here, so this sets the XOR of all 16 to 0
  out[2] = xor;
  return out;
}

// Parse an LDR into its records: [{offset, code, target, count, argument, fill, stored}]. Same bounds and
// checks as formats.py `ldr`: 0xad top byte, XOR 0, FINAL (bit 0x8000) ends the file with nothing after.
export function parseLdr(data) {
  req(data.length > 0 && data.length <= 16 * 1024 * 1024, "LDR size outside supported bounds");
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const blocks = [];
  let offset = 0;
  while (offset < data.length) {
    req(blocks.length < 10000 && offset + 16 <= data.length, "invalid LDR block boundary");
    let xor = 0;
    for (let i = 0; i < 16; i++) xor ^= data[offset + i];
    const code = dv.getUint32(offset, true);
    const target = dv.getUint32(offset + 4, true);
    const count = dv.getUint32(offset + 8, true);
    const argument = dv.getUint32(offset + 12, true);
    req((code >>> 24) === 0xad && xor === 0, "invalid LDR header/checksum");
    req(target + count <= 0x100000000, "LDR address overflow");
    const fill = (code & 0x100) !== 0;
    const stored = fill ? 0 : count;
    req(offset + 16 + stored <= data.length, "LDR payload exceeds file");
    blocks.push({ offset, code, target, count, argument, fill, stored });
    offset += 16 + stored;
    if (code & 0x8000) { req(offset === data.length, "trailing bytes after FINAL"); return blocks; }
  }
  throw new Error("missing FINAL record");
}

// The file offset an address maps to: exactly one record (fill records counted) may overlap, it must be
// stored and must fully contain the range.
export function mappedOffset(blocks, address, size) {
  req(size > 0, "empty address range");
  const overlaps = blocks.filter((b) => b.target < address + size && address < b.target + b.count);
  req(overlaps.length === 1, "ambiguous load mapping");
  const b = overlaps[0];
  req(!b.fill && b.target <= address && address + size <= b.target + b.count, "not a stored payload interval");
  return b.offset + 16 + address - b.target;
}

export const FIRST_FLAGS = 0x5004, FINAL_FLAGS = 0x8004, RECORD_FLAGS = 4;
