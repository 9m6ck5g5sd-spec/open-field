// open-field: the listing of packs, as the install page reads it, and the download of a ticked pack.
//
// The listing is catalog/packs.json of the repository; the host's pages put a copy beside the page as packs.json
// (.github/workflows/pages.yml). tools/catalog_check.py holds every entry to its file before it is merged; this
// module reads only what the page needs, and holds every download to its entry once more.
//
// A ticked pack is downloaded from its author's repository at the commit the listing names: a file of a repository at
// a commit is one a web page may read (raw.githubusercontent.com answers any page), and that address never changes
// when the author pushes again. The file is held to the entry's size and SHA-256 before it goes anywhere near the
// folder. This is the only place where the page loads anything from another site.
//
// Runs in the browser and in Node (no Node APIs in this file), so that validate-install.mjs runs the same code.

import { sha256 } from "./install.mjs";

export const LISTING_FILE = "packs.json";                     // beside the page, put there by the host's pages
export const RAW = "https://raw.githubusercontent.com/";       // the only site the page downloads from
export const OURS = "open-field";                              // the author of open-field's own packs
export const NOTICE = "A pack is a program that can read and write the files on your unit: take packs only from people you trust.";

const REPOSITORY = /^https:\/\/github\.com\/([A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?)\/([A-Za-z0-9._-]{1,100})$/;
const COMMIT = /^[0-9a-f]{40}$/;
const SHA256 = /^[0-9a-f]{64}$/;
const ID = /^[\x20-\x7e]{1,15}$/;
const PATH_PART = /^[A-Za-z0-9 _.()+-]+$/;

function pathOk(path) {
  if (typeof path !== "string" || !/\.pack$/i.test(path)) return false;
  return path.split("/").every((part) => part && part !== "." && part !== ".." && PATH_PART.test(part));
}

// An id the page may name a file after on the unit: `<id>.pack`.
function idOk(id) {
  return typeof id === "string" && ID.test(id) && !/[\\/:*?"<>|]/.test(id) && !/^[ .]|[ .]$/.test(id);
}

// The link a listed entry's file is downloaded from: its repository, at its commit, its path.
export function rawLink(entry) {
  const found = REPOSITORY.exec(entry.repository);
  if (!found) throw new Error(entry.id + ": its repository is not a GitHub repository's address");
  return RAW + found[1] + "/" + found[2] + "/" + entry.commit + "/" + entry.path.split("/").map(encodeURIComponent).join("/");
}

// The name the pack's file takes in the folder's packs and on the unit.
export const fileName = (entry) => entry.id + ".pack";

// What the page offers from a listing, for the unit's firmware (the loader's manifest names it, "1.7.3"):
// {base, offered: Map id -> entry, categories: [{key, label, entries}], ours: Set, otherFirmware, problems}.
// Only `listed` entries are offered, each once, under its first category; `bundled` base.pack is shown apart, as it
// goes on every unit. An entry the page cannot use (a field out of form) is left out and named in `problems`.
export function readListing(data, firmware) {
  if (!data || typeof data !== "object" || !Array.isArray(data.packs) || !data.categories || typeof data.categories !== "object") {
    throw new Error("the listing of packs is not in form");
  }
  const ours = new Set(Array.isArray(data.ours) ? data.ours.filter((r) => typeof r === "string") : []);
  const offered = new Map();
  const problems = [];
  let base = null, otherFirmware = 0;
  for (const entry of data.packs) {
    if (!entry || typeof entry !== "object") continue;
    if (entry.status === "bundled" && entry.id === "base") { base = entry; continue; }
    if (entry.status !== "listed") continue;
    const fine = idOk(entry.id) && REPOSITORY.test(entry.repository ?? "") && COMMIT.test(entry.commit ?? "") && pathOk(entry.path) &&
      SHA256.test(entry.sha256 ?? "") && Number.isInteger(entry.bytes) && entry.bytes > 0 && Number.isInteger(entry.version) &&
      Array.isArray(entry.categories) && entry.categories.length > 0 && entry.categories.every((c) => c in data.categories) &&
      typeof entry.description === "string" && typeof entry.author === "string" && Array.isArray(entry.needs ?? []);
    if (!fine) { problems.push((typeof entry.id === "string" ? entry.id : "an entry") + ": not in form, left out"); continue; }
    if ([...offered.keys()].some((id) => id.toLowerCase() === entry.id.toLowerCase())) { problems.push(entry.id + ": named twice, left out"); continue; }
    if (entry.firmware !== firmware) { otherFirmware++; continue; }
    offered.set(entry.id, entry);
  }
  const categories = Object.entries(data.categories).map(([key, label]) => ({ key, label: String(label), entries: [] }));
  for (const entry of offered.values()) categories.find((c) => c.key === entry.categories[0]).entries.push(entry);
  return { base, offered, categories: categories.filter((c) => c.entries.length), ours, otherFirmware, problems };
}

// Whether an entry is one of open-field's own packs: by open-field, in one of the listing's own repositories. The
// page shows NOTICE on every other.
export function isOurs(listing, entry) {
  return entry.author === OURS && listing.ours.has(entry.repository);
}

// The packs a pack cannot run without, besides base.pack (which goes on every unit), as ids: {ids, missing}.
// `missing` names what a ticked pack needs and the listing does not offer.
export function withNeeds(listing, ticked) {
  const ids = new Set(), missing = new Set(), todo = [...ticked];
  while (todo.length) {
    const id = todo.pop();
    if (ids.has(id)) continue;
    const entry = listing.offered.get(id);
    if (!entry) { missing.add(id); continue; }
    ids.add(id);
    for (const need of entry.needs ?? []) if (need !== "base") todo.push(need);
  }
  return { ids, missing };
}

// The ticked packs that need `id`, besides itself.
export function neededBy(listing, ticked, id) {
  return [...ticked].filter((other) => other !== id && withNeeds(listing, [other]).ids.has(id));
}

// A listed entry's file, downloaded at its commit and held to the entry: {id, name, bytes, sha256, from}, or an error
// that names the pack and what was wrong. `get` is the browser's fetch; a test gives its own.
export async function download(entry, get = (url, options) => fetch(url, options)) {
  const url = rawLink(entry);
  let answer;
  try { answer = await get(url, { credentials: "omit", redirect: "error" }); }
  catch (e) { throw new Error(entry.id + ": could not be downloaded (" + (e.message || e) + ")"); }
  if (!answer.ok) throw new Error(entry.id + ": the download answered " + answer.status + ", not the file");
  const bytes = new Uint8Array(await answer.arrayBuffer());
  if (bytes.length !== entry.bytes) throw new Error(entry.id + ": the file downloaded has " + bytes.length + " bytes, the listing says " + entry.bytes + ": refused");
  const digest = await sha256(bytes);
  if (digest !== entry.sha256) throw new Error(entry.id + ": the file downloaded is not the one the listing names (its SHA-256 differs): refused");
  return { id: entry.id, name: fileName(entry), bytes, sha256: digest, from: url };
}
