// open-field: the install page's live list of packs, as pure logic. What is on the unit (the door's list and the
// SHA-256 of each file) is held against the listing (catalog.mjs) and the page's own base.pack, a row a pack; the
// owner's ticks become a plan of files to send and packs to take off. No browser object and no Node API in this file:
// validate-packs-live.mjs runs the same code as the page.
//
// The rules the page keeps (the plan of the packs list, decided before it was built):
// 1. A tick means "on the unit after the change". What is on the unit comes ticked, and unticking it marks it to come
//    off. A listed pack that is not on the unit comes unticked, and ticking it adds it.
// 2. An update has its own button and is made only when that is pressed.
// 3. The base and what it needs travel together: when the page brings a new base, every listed pack on the unit moves
//    to the listing's file in the same change, ticked and locked ("comes with the new base"). A pack the listing does
//    not know is kept as it is.
// 4. The base cannot be unticked.
// 5. A pack another needs is ticked with it, and stays ticked while the other does.
// 6. At most 32 pack files after the change, and a name the loader would not take is never offered.
// 7. The plan: the removals, then the files to send with base.pack last (the unit moves it last as well).

import { checkName } from "./door.mjs";
import { fileName, withNeeds, neededBy, entryForFile } from "./catalog.mjs";

export const MOST_PACKS = 32;
export const BASE = "base.pack";
export const STATE = Object.freeze({ on: "on", update: "update", different: "different", absent: "absent", notListed: "not listed", leftOut: "left out" });
export const ACTION = Object.freeze({ keep: "keep", add: "add", update: "update", remove: "remove", none: "none" });

const key = (name) => String(name).toLowerCase();

// The unit's files from the door: the list of user/pk and the hashes read, as { name, size, sha256 (or null), isPack, sizeUnknown, nameCut, hashError }.
export function unitFiles(list, hashes = []) {
  const byName = new Map(hashes.map((h) => [h.name, h]));
  return ((list && list.entries) || []).map((e) => {
    const h = byName.get(e.name);
    return { name: e.name, size: e.sizeUnknown ? null : e.size, sha256: h && h.sha256 ? h.sha256.toLowerCase() : null, isPack: !!e.isPack && !e.nameCut, sizeUnknown: !!e.sizeUnknown, nameCut: !!e.nameCut, hashError: h && h.error ? h.error : null,
      // the file's own header, from a door of build 5 or later (protocol section 14): null when not asked or not read
      headerVersion: e.headerRead ? e.version : null, headerName: e.headerRead ? e.packName : null, headerOsId: e.headerRead ? e.osId : null };
  });
}

// The rows: { base, listed: [...], others: [...] }. Each row: { key, kind, name, entry, unit, state, unitVersion, offerVersion }.
// listing: catalog.mjs's readListing() for the page's firmware. base: the page's own base.pack { sha256, size, version, release }.
// unit: { hello: { baseVersion, release }, files: unitFiles(...) }. known: catalog.mjs's knownVersions() (with the page's own records added).
export function readRows({ listing, base, unit, known = new Map() }) {
  const files = new Map((unit.files || []).map((f) => [key(f.name), f]));
  const used = new Set();
  // a file whose own header names another firmware than the unit's: the loader leaves it out (the review of base 25, S-4)
  const unitOs = unit.hello && unit.hello.osId !== undefined && unit.hello.osId !== null ? unit.hello.osId : null;
  const otherFirmware = (f) => !!f && unitOs !== null && f.headerOsId !== null && f.headerOsId !== undefined && f.headerOsId !== unitOs;
  // the base: its version is the unit's own word (hello), its file the page's own
  const baseFile = files.get(BASE) ?? null;
  if (baseFile) used.add(BASE);
  let baseState = STATE.absent;
  if (baseFile) {
    if (base && baseFile.sha256 && baseFile.sha256 === base.sha256) baseState = STATE.on;
    else if (base && unit.hello && base.version > unit.hello.baseVersion) baseState = STATE.update;
    else baseState = STATE.different;
  }
  const baseRow = { key: BASE, kind: "base", name: baseFile ? baseFile.name : BASE, entry: listing ? listing.base : null, unit: baseFile, state: baseState,
    unitVersion: unit.hello ? unit.hello.baseVersion : null, unitRelease: unit.hello ? unit.hello.release : null, offerVersion: base ? base.version : null, offerRelease: base ? base.release : null };
  // the listed packs, in the listing's order (its categories)
  const listed = [];
  for (const category of listing ? listing.categories : []) {
    for (const entry of category.entries) {
      const k = key(fileName(entry));
      const f = files.get(k) ?? null;
      if (f) used.add(k);
      let state = STATE.absent, unitVersion = null;
      if (f) {
        const knownAs = f.sha256 ? known.get(f.sha256) : null;
        if (knownAs && knownAs.id === entry.id) unitVersion = knownAs.version;
        // a file the page does not know by its fingerprint: the version its own header says, when it says the same pack
        else if (f.headerVersion !== null && f.headerVersion !== undefined && f.headerName === entry.id && !otherFirmware(f)) unitVersion = f.headerVersion;
        if (f.sha256 && f.sha256 === entry.sha256 && f.size === entry.bytes) { state = STATE.on; unitVersion = entry.version; }
        else if (unitVersion !== null && unitVersion < entry.version) state = STATE.update;
        else state = STATE.different;
      }
      listed.push({ key: k, kind: "listed", name: f ? f.name : fileName(entry), entry, category: category.key, unit: f, state, unitVersion, offerVersion: entry.version,
        nameProblem: checkName(fileName(entry)), otherFirmware: otherFirmware(f) });
    }
  }
  // what is on the unit and not in the listing: the examples, a friend's pack, a test build
  const others = [];
  for (const f of unit.files || []) {
    const k = key(f.name);
    if (used.has(k)) continue;
    const knownAs = f.sha256 ? known.get(f.sha256) : null;
    others.push({ key: k, kind: "other", name: f.name, entry: null, unit: f, state: f.isPack ? STATE.notListed : STATE.leftOut,
      unitVersion: knownAs ? knownAs.version : (f.headerVersion ?? null), knownId: knownAs ? knownAs.id : null, otherFirmware: otherFirmware(f) });
  }
  return { base: baseRow, listed, others };
}

// The ticks as the page starts: every file on the unit ticked, no update asked.
export function initialChoices(rows) {
  const ticked = new Set();
  for (const r of rows.listed) if (r.unit) ticked.add(r.key);
  for (const r of rows.others) if (r.state === STATE.notListed) ticked.add(r.key);
  return { ticked, updates: new Set() };
}

// Whether a row's update button is offered: an update out, or a file that is not the listing's (the listing's file in its place).
export function updateOffered(row) {
  if (row.kind === "base") return row.state === STATE.update;
  return row.kind === "listed" && (row.state === STATE.update || row.state === STATE.different);
}

// The rules applied to the owner's choices: { view: Map key -> { ticked, locked, why, action }, plan, problems, counts, baseComes }.
// plan = { send: [{ key, name, id, version, kind: "add" | "update" | "base", entry, bytes, sha256 }], remove: [names] }, base.pack last.
export function resolve(rows, choices, listing, base) {
  const view = new Map();
  const problems = [];
  const baseComes = updateOffered(rows.base) && choices.updates.has(BASE);
  // rule 3: a new base brings every listed pack on the unit to the listing's file
  const forced = new Set();
  if (baseComes) for (const r of rows.listed) if (r.unit) forced.add(r.key);
  // the ids the owner ticked (rule 1), the forced ones, and what they need (rule 5)
  const blocked = new Map();
  for (const r of rows.listed) {
    if (r.nameProblem) { blocked.set(r.key, "its name won't do on your OP-1"); continue; }
    const missing = (r.entry.needs ?? []).filter((n) => n !== "base" && !listing.offered.has(n));
    if (missing.length) blocked.set(r.key, "it needs " + missing.join(", ") + ", which the listing doesn't offer");
  }
  const own = rows.listed.filter((r) => !blocked.has(r.key) && (choices.ticked.has(r.key) || forced.has(r.key))).map((r) => r.entry.id);
  const wanted = withNeeds(listing, own).ids;
  for (const r of rows.listed) {
    const id = r.entry.id;
    if (blocked.has(r.key) && !r.unit) { view.set(r.key, { ticked: false, locked: true, why: blocked.get(r.key), action: ACTION.none }); continue; }
    const ticked = wanted.has(id) || (blocked.has(r.key) && choices.ticked.has(r.key));
    const needers = neededBy(listing, own, id);
    let locked = false, why = "";
    if (forced.has(r.key)) { locked = true; why = r.state === STATE.on ? "already the one that goes with the new base" : "comes with the new base"; }
    else if (needers.length && ticked) { locked = true; why = needers.join(", ") + " needs it"; }
    let action;
    if (r.unit) action = !ticked ? ACTION.remove : (forced.has(r.key) && r.state !== STATE.on) || (choices.updates.has(r.key) && updateOffered(r)) ? ACTION.update : ACTION.keep;
    else action = ticked ? ACTION.add : ACTION.none;
    view.set(r.key, { ticked, locked, why, action });
  }
  for (const r of rows.others) {
    if (r.state === STATE.leftOut) { view.set(r.key, { ticked: true, locked: true, why: "the loader doesn't take its name, so your OP-1 leaves it out", action: ACTION.keep }); continue; }
    const ticked = choices.ticked.has(r.key);
    view.set(r.key, { ticked, locked: false, why: "", action: ticked ? ACTION.keep : ACTION.remove });
  }
  view.set(BASE, { ticked: true, locked: true, why: "every pack runs on it", action: baseComes ? ACTION.update : ACTION.keep });
  // the plan
  const send = [], remove = [];
  for (const r of rows.listed) {
    const v = view.get(r.key);
    if (v.action === ACTION.remove) remove.push(r.unit.name);
    // an update keeps the unit's own name for the file, whatever its capitals, so that it takes the old file's place
    if (v.action === ACTION.add || v.action === ACTION.update) {
      send.push({ key: r.key, name: r.unit ? r.unit.name : fileName(r.entry), id: r.entry.id, version: r.entry.version, kind: v.action, entry: r.entry, bytes: r.entry.bytes, sha256: r.entry.sha256, from: r.unitVersion });
    }
  }
  for (const r of rows.others) if (view.get(r.key).action === ACTION.remove) remove.push(r.unit.name);
  if (baseComes) send.push({ key: BASE, name: BASE, id: "base", version: base.version, kind: "base", entry: null, bytes: base.size, sha256: base.sha256, from: rows.base.unitVersion });
  // rule 6: the pack files after the change
  const onUnit = [rows.base, ...rows.listed, ...rows.others].filter((r) => r.unit && r.unit.isPack).map((r) => r.unit.name.toLowerCase());
  const after = new Set(onUnit.filter((n) => !remove.some((x) => x.toLowerCase() === n)));
  for (const s of send) after.add(s.name.toLowerCase());
  if (after.size > MOST_PACKS) problems.push("That would leave " + after.size + " packs on your OP-1, and it loads " + MOST_PACKS + " at most. Untick a few.");
  const counts = {
    added: send.filter((s) => s.kind === "add").length,
    updated: send.filter((s) => s.kind !== "add").length,
    removed: remove.length,
    bytes: send.reduce((n, s) => n + (s.bytes || 0), 0),
    packsAfter: after.size,
  };
  return { view, plan: { send, remove }, problems, counts, baseComes };
}

// The changes waiting, as short lines for the bar: [{ sign: "+" | "↑" | "−", text }].
export function changeLines(rows, resolved) {
  const out = [];
  const name = (s) => s.replace(/\.pack$/i, "");
  for (const s of resolved.plan.send) {
    if (s.kind === "add") out.push({ sign: "+", text: s.id });
    else if (s.kind === "base") out.push({ sign: "↑", text: "base " + (s.from ?? "?") + " → " + s.version });
    else out.push({ sign: "↑", text: s.id + (s.from !== null && s.from !== undefined ? " " + s.from + " → " + s.version : " → v" + s.version) });
  }
  const sent = new Set(resolved.plan.send.map((s) => s.name.toLowerCase()));
  for (const r of resolved.plan.remove) if (!sent.has(r.toLowerCase())) out.push({ sign: "−", text: name(r) });
  return out;
}

// About how long a change takes over USB, in whole seconds: the files at about 250 kB a second, their check on the unit, and the asking.
export function secondsFor(bytes) { return Math.max(3, Math.round(bytes / 250000 + bytes / 2000000 + 2)); }

// The short words of a row's state, for its tile.
export function stateWords(row, v) {
  if (v && v.action === ACTION.remove) return "coming off";
  if (row.otherFirmware) return "made for another OS version, so your OP-1 leaves it out";
  switch (row.state) {
    case STATE.on: return "on your OP-1";
    case STATE.update: return (row.unitVersion !== null ? "v" + row.unitVersion + " on your OP-1 · " : "") + "v" + row.offerVersion + " is out";
    case STATE.different:
      // the owner's ask (2026-11-20): when a change is offered, say plainly which version is on the unit and which the list has
      if (row.unitVersion === null || row.unitVersion === undefined) return "a different file on your OP-1 · the list has v" + row.offerVersion;
      if (row.unitVersion < row.offerVersion) return "v" + row.unitVersion + " on your OP-1 · v" + row.offerVersion + " is out";
      if (row.unitVersion > row.offerVersion) return "v" + row.unitVersion + " on your OP-1 · the list has an older v" + row.offerVersion;
      return "v" + row.unitVersion + " on your OP-1, not the list's file";
    case STATE.absent: return "not on it";
    case STATE.notListed: return "on your OP-1, not in the list";
    case STATE.leftOut: return "left out at the start";
    default: return "";
  }
}

// The colour of a row's dot, by the four knobs: blue on the unit, ochre an update, grey not on it, red coming off.
export function stateColour(row, v) {
  if (v && v.action === ACTION.remove) return "red";
  if (row.state === STATE.update) return "ochre";
  if (row.state === STATE.absent || row.state === STATE.leftOut || row.otherFirmware) return "grey";
  return "blue";
}

// The base's words for the header: its release on the unit and the page's.
export function baseWords(rows) {
  const b = rows.base;
  if (b.state === STATE.on) return "open-field " + (b.unitRelease || "?") + " on your OP-1";
  if (b.state === STATE.update) return "open-field " + (b.unitRelease || "?") + " on your OP-1 · " + b.offerRelease + " is out";
  if (b.state === STATE.different) return "open-field " + (b.unitRelease || "?") + " on your OP-1, a base this page didn't bring" + (b.offerRelease ? " (this page has " + b.offerRelease + ")" : "");
  return "no base.pack on your OP-1";
}

export { entryForFile };
