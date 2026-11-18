// Checks builder/ferret.mjs, the session behind step 1 of the install page, against a simulated te-boot.
// The simulated unit answers as a real unit did on 2026-11-15 (te-boot 1000.1.1, read by the dev-mode page in
// Chrome on Windows): the same field names, the same port names, no serial of a real unit. No MIDI port of the
// computer is touched: the session is handed a made-up MIDIAccess.
//   node builder/validate-devmode.mjs
// What it cannot hold: the page's own glue in index.html and a real unit's timing.
import { createHash } from "node:crypto";
import { Session, encode, decode, enablePayload, disablePayload, crc8 } from "./ferret.mjs";

let fails = 0, checks = 0;
const ok = (m) => { console.log("  ok   " + m); checks++; };
const bad = (m) => { console.error("  FAIL " + m); fails++; };
const same = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const hex = (a) => Array.from(a, (v) => v.toString(16).padStart(2, "0")).join(" ");

// What te-boot 1000.1.1 said of itself, the two serial numbers made up.
const FIELDS = { mode: "bootloader", serial: "SIMULATED0001", dsp_serial: "000000000000000001", hw_rev: "6", product: "OP-1",
  sku: "TE002AS002", sw_version: "1.7.0", bl_version: "1000.1.1", os_version: "1.7.0" };
const metaText = (fields) => Object.entries(fields).map(([k, v]) => k + ":" + v).join(";") + ";";
const CHROME = ["OP-1", "Teenage Engineering"];                 // the port as Chrome reports it at the te-boot menu
const WINDOWS = ["OP-1 (Composite)", "Microsoft Corporation"];  // the same port by the Windows multimedia API's name
const OTHERS = [["Some Controller", "Microsoft Corporation"], ["Some Synth A", "Microsoft Corporation"], ["Some Synth B", "Microsoft Corporation"]];   // a computer's other ports, their names made up
const ROT = [0x99, 0x4a, 0xf3, 0xed];

// A made-up MIDIAccess: the computer's other ports, and one te-boot for every entry of `units`.
function world({ units = [CHROME], fields = FIELDS, challenge = 16, silent = false } = {}) {
  const state = { sent: [], fields, challenge, silent };
  const inputs = new Map(), outputs = new Map();
  const port = (id, [name, manufacturer]) => ({ id, name, manufacturer, state: "connected", connection: "closed",
    open: async () => {}, close: async () => {} });
  OTHERS.forEach((names, i) => {
    const input = port("other-in-" + i, names); input.addEventListener = () => {}; input.removeEventListener = () => {};
    const output = port("other-out-" + i, names); output.send = () => { state.sent.push({ stray: names[0] }); };
    inputs.set(input.id, input); outputs.set(output.id, output);
  });
  units.forEach((names, i) => {
    const listeners = new Set();
    const input = port("unit-in-" + i, names);
    input.addEventListener = (type, fn) => { if (type === "midimessage") listeners.add(fn); };
    input.removeEventListener = (type, fn) => { listeners.delete(fn); };
    const output = port("unit-out-" + i, names);
    output.send = (bytes) => {
      const frame = decode(bytes);
      state.sent.push({ type: frame.type, payload: Array.from(frame.payload), raw: Array.from(bytes) });
      if (state.silent) return;
      let reply = null;
      if (frame.type === 0x09) reply = encode(0x0a, new TextEncoder().encode(metaText(state.fields)));
      if (frame.type === 0x1f) reply = encode(0x20, Uint8Array.from({ length: state.challenge }, (_, k) => 0x10 + k));
      if (reply) setTimeout(() => listeners.forEach((fn) => fn({ data: reply, target: input })), 2);
    };
    inputs.set(input.id, input); outputs.set(output.id, output);
  });
  state.access = { inputs, outputs, addEventListener() {}, removeEventListener() {} };
  return state;
}

function session(state, seen = {}) {
  return new Session(async (options) => { seen.options = options; return state.access; }, () => {}, 250,
    { onPorts: (ports) => { seen.ports = ports; }, onProgress: (line) => { (seen.progress ??= []).push(line); } });
}

async function refused(promise) {
  try { await promise; return null; } catch (error) { return String(error.message || error); }
}

const wanted = (enable) => [...ROT, ...Array.from({ length: 16 }, (_, k) => (0x10 + k) ^ ROT[k % 4]), 0xba, enable ? 0xa7 : 0x00];

// 1. the wire: the Field's header, seven-bit bytes, and a frame read back
{
  const frame = encode(0x09);
  if (same(Array.from(frame.slice(0, 6)), [0xf0, 0x00, 0x20, 0x76, 0x02, 0x66]) && frame.at(-1) === 0xf7 && frame.slice(1, -1).every((b) => b < 0x80)) {
    ok("a command goes out as F0 00 20 76 02 66 … F7 with seven-bit bytes (" + hex(frame) + ")");
  } else bad("the frame of command 0x09 is not the Field's: " + hex(frame));
  const payload = Uint8Array.from({ length: 200 }, (_, k) => (k * 37) & 0xff);
  const back = decode(encode(0x0a, payload));
  if (back && back.type === 0x0a && same(Array.from(back.payload), Array.from(payload))) ok("a frame of 200 bytes with every kind of byte reads back as it was written");
  else bad("a frame does not read back");
  if (decode(Uint8Array.from([0xf0, 0x00, 0x20, 0x76, 0x21, 0x66, 0x00, 0x01, 0x02, 0xf7])) === null) ok("a frame under the OP-XY's header is not taken");
  else bad("a frame under another header was taken");
}

// 1b. the frame check's table, which ferret.mjs computes where the upstream writes out 256 bytes
{
  // Entry b of the table is the check of the single byte b, so the table is read through the function the frames
  // are made with. The 256 bytes are not written here either: the same table stands in the unit's firmware, and a
  // file meant to be published does not carry it. What is written is their SHA-256, taken with Python's hashlib
  // from the string ferret.mjs had until 2026-11-16, which is the upstream's; that day the computed table and the
  // string were also held against each other byte by byte, and 4,000 frames made with each.
  const UPSTREAM_TABLE_SHA256 = "9628cf162c7f92519aa84ac45780e7e1e3956cbed134bca73f413b6eda395357";
  const table = Uint8Array.from({ length: 256 }, (_, b) => crc8(Uint8Array.of(b)));
  const sum = createHash("sha256").update(table).digest("hex");
  if (sum === UPSTREAM_TABLE_SHA256) ok("the frame check's table, computed, is the 256 bytes the upstream writes out (SHA-256 " + sum.slice(0, 16) + "…)");
  else bad("the computed table of the frame check is not the upstream's: its SHA-256 is " + sum);
  // The two read-only queries, byte for byte as this code has sent them since 2026-11-14. On 2026-11-15 te-boot
  // 1000.1.1 answered both, so it takes the check byte in each.
  const ANSWERED = [[0x09, "f0 00 20 76 02 66 20 02 21 02 09 02 18 00 f7"], [0x1f, "f0 00 20 76 02 66 20 02 21 02 1f 02 24 00 f7"]];
  const other = ANSWERED.filter(([command, bytes]) => hex(encode(command)) !== bytes);
  if (other.length === 0) ok("the two read-only queries go out byte for byte as a unit has answered them (commands 0x09 and 0x1F)");
  else bad("a query frame is not the one a unit answered: " + other.map(([command, bytes]) => "0x" + command.toString(16).padStart(2, "0") + " goes out as " + hex(encode(command)) + ", was " + bytes).join("; "));
}

// 2. the port under Chrome's name, and what the session hands the page
{
  const state = world(), seen = {};
  const s = session(state, seen);
  const id = await s.connect();
  if (id.bl_version === "1000.1.1" && id.serial === FIELDS.serial && id.mode === "bootloader" && id.product === "OP-1") ok("finds the port Chrome calls \"OP-1\" and reads the identity (bl_version 1000.1.1)");
  else bad("identity not as the unit gives it: " + JSON.stringify(id));
  if (seen.options && seen.options.sysex === true) ok("asks the browser for MIDI with SysEx");
  else bad("did not ask for SysEx");
  const names = (seen.ports?.outputs || []).map((p) => p.name);
  if (names.length === OTHERS.length + 1 && names.includes("OP-1") && seen.ports.inputs.length === names.length) ok("reports every port the browser lists, for the page's log (" + names.length + " in each direction)");
  else bad("the ports were not reported: " + JSON.stringify(seen.ports));
  if (state.sent.length === 1 && state.sent[0].type === 0x09 && state.sent[0].payload.length === 0) ok("sends one read-only query, and nothing to any other port");
  else bad("sent on connecting: " + JSON.stringify(state.sent.map((f) => f.type ?? f.stray)));

  // 3. enable and disable, byte for byte the original's formula
  await s.enable();
  const types = state.sent.map((f) => f.type);
  if (same(types, [0x09, 0x09, 0x1f, 0xac])) ok("enable reads the identity again, reads the challenge, then sends the command");
  else bad("enable sent " + types.map((t) => "0x" + t.toString(16)).join(" "));
  const command = state.sent.at(-1).payload;
  if (same(command, wanted(true)) && same(command, Array.from(enablePayload(Uint8Array.from({ length: 16 }, (_, k) => 0x10 + k))))) ok("the enable command is 99 4A F3 ED, the challenge under that key, BA, A7 (" + command.length + " bytes)");
  else bad("enable payload " + hex(command));
  await s.disable();
  const off = state.sent.at(-1).payload;
  if (same(off, wanted(false)) && same(off, Array.from(disablePayload(Uint8Array.from({ length: 16 }, (_, k) => 0x10 + k))))) ok("the disable command differs in its last byte alone (00)");
  else bad("disable payload " + hex(off));

  // 4. a longer answer to the challenge: its first sixteen bytes, as the Field's own Ferret takes it
  state.challenge = 18;
  await s.enable();
  if (same(state.sent.at(-1).payload, wanted(true))) ok("a challenge reply of 18 bytes is taken by its first 16");
  else bad("an 18-byte challenge gave " + hex(state.sent.at(-1).payload));
  state.challenge = 15;
  const before = state.sent.filter((f) => f.type === 0xac).length;
  const short = await refused(s.enable());
  if (short && /16-byte challenge/.test(short) && state.sent.filter((f) => f.type === 0xac).length === before) ok("a challenge reply of 15 bytes sends no command: \"" + short + "\"");
  else bad("a 15-byte challenge: " + short);
  s.close();
}

// 5. the same port under the name Windows' own list gives it
{
  const s = session(world({ units: [WINDOWS] }));
  const id = await s.connect();
  if (id.bl_version === "1000.1.1") ok("finds the port under the name \"OP-1 (Composite)\" too");
  else bad("the port named OP-1 (Composite) was not taken");
  s.close();
}

// 6. no unit, and two units
{
  const none = await refused(session(world({ units: [] })).connect());
  if (none && /no connected OP-1 Field MIDI output/.test(none) && /listed in the log/.test(none) && !/Connection help/.test(none)) ok("no port of a unit: refused, and the text points at the log");
  else bad("no unit: " + none);
  const state = world({ units: [CHROME, WINDOWS] });
  const two = await refused(session(state).connect());
  if (two && /More than one OP-1 Field/.test(two) && state.sent.length === 0) ok("two units: refused before anything is sent");
  else bad("two units: " + two);
}

// 7. an identity that is not te-boot's
{
  const { bl_version, ...without } = FIELDS;
  const text = await refused(session(world({ fields: without })).connect());
  if (text && /identity not confirmed/.test(text) && /mode:bootloader/.test(text) && /\bserial\b/.test(text) && !text.includes(FIELDS.serial) && !text.includes(FIELDS.dsp_serial)) ok("no bl_version: refused, the fields that did answer named, no serial's value in the text");
  else bad("no bl_version: " + text);
  const normal = await refused(session(world({ fields: { ...FIELDS, mode: "normal" } })).connect());
  if (normal && /identity not confirmed/.test(normal)) ok("a unit that is not in its boot menu (mode:normal) is refused");
  else bad("mode normal: " + normal);
}

// 8. a unit that does not answer, and a unit swapped after Find
{
  const silent = await refused(session(world({ silent: true })).connect());
  if (silent && /No reply/.test(silent)) ok("no answer within the time: \"" + silent + "\"");
  else bad("silent unit: " + silent);
  const state = world();
  const s = session(state);
  await s.connect();
  state.fields = { ...FIELDS, serial: "SIMULATED0002" };
  const swapped = await refused(s.enable());
  if (swapped && /Connected device changed/.test(swapped) && !state.sent.some((f) => f.type === 0xac)) ok("another unit on the cable after Find: no command is sent");
  else bad("swapped unit: " + swapped);
}

console.log(`\n${fails === 0 ? "OK" : "FAILED"}: ${checks} checks, ${fails} failures`);
process.exit(fails === 0 ? 0 : 1);
