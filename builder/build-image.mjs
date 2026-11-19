// The builder as a command: your own firmware file in, the open-field firmware file out. It does what
// the install page (index.html) does in the browser, with the same code (patch.mjs). Nothing is uploaded.
//
//   node builder/build-image.mjs <your own os.ldr> <the file to write>
//   node builder/build-image.mjs --loader <os>-<version> <your own os.ldr> <the file to write>
//
// Your file must be the exact OS 1.7.3 file (the original, 1.7.3+0); any other is refused. The result is written only when its SHA-256 is
// the one the manifest names, and never over an existing file. Keep your own os.ldr: it is the way back.
//
// The file it adds to yours is the loader, our small piece of code (builder/loader-<os>-<version>.bin). Without
// --loader the manifest is loader.json: the version the page offers by itself. With it, a version by its name:
// --loader 1.7.3-v2 takes loader-1.7.3-v2.json, the manifest of a version kept beside that one, as
// index.html?loader=1.7.3-v2 does; or loader.json, when that is the version it names. The line this command prints
// at the end says whether that version's firmware file has run on a unit.
import { readFileSync, existsSync, writeFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { buildImage, ours } from "./patch.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const words = process.argv.slice(2);
let manifestName = "loader.json";
if (words[0] === "--loader") {
  const asked = words[1] || "";
  if (!/^[0-9]+(\.[0-9]+)*-v[0-9]+$/.test(asked)) { console.error("no such loader: " + asked); process.exit(2); }
  if (existsSync(join(here, "loader-" + asked + ".json"))) manifestName = "loader-" + asked + ".json";
  else {
    const own = JSON.parse(readFileSync(join(here, "loader.json"), "utf8"));
    if (`${own.os}-${ours(own).version}` !== asked) { console.error("no manifest for the loader " + asked + " in builder/"); process.exit(2); }
  }
  words.splice(0, 2);
}
const [stockPath, outPath] = words;
if (!stockPath || !outPath || words.length !== 2) {
  console.error("usage: node builder/build-image.mjs [--loader <os>-<version>] <your own os.ldr> <the file to write>");
  process.exit(2);
}
if (!existsSync(stockPath)) { console.error("no such file: " + stockPath); process.exit(2); }
if (existsSync(outPath)) { console.error(outPath + " exists: nothing is overwritten"); process.exit(2); }

const manifest = JSON.parse(readFileSync(join(here, manifestName), "utf8"));
const our = ours(manifest);
const loader = new Uint8Array(readFileSync(join(here, our.file)));
try {
  const { image, report } = await buildImage(new Uint8Array(readFileSync(stockPath)), manifest, loader);
  writeFileSync(outPath, image, { flag: "wx" });
  console.log(`written: ${outPath}`);
  console.log(`  ${image.length} bytes, SHA-256 ${report.output_sha256}`);
  console.log(`  loader ${our.version} (${our.status || "no status line"}), for OS ${manifest.os}`);
  console.log("Keep your own os.ldr. On the unit this file must be named os.ldr.");
} catch (e) {
  console.error("refused: " + e.message);
  process.exit(1);
}
