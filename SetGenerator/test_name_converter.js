"use strict";
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm"), assert = require("node:assert/strict");
const html = fs.readFileSync(path.join(__dirname, "Dominion-Generator.html"), "utf8");
const readScript = id => JSON.parse(html.match(new RegExp('id="' + id + '">([\\s\\S]+?)</script>'))[1]);
const names = readScript("web-card-names"), catalog = readScript("catalog");
const byId = new Map(catalog.cards.map(card => [card.id, card]));
const sourceFunction = name => html.match(new RegExp("(?:async )?function " + name + "\\([\\s\\S]*?\\n\\}"))[0];
const context = {
  uiLanguage: "de", cardById: byId, cardTitle: (card, language) => card.text[language][0],
  interfaceLabels: [], currentResult: Object.freeze({main: ["village"]}),
  generatorOutput: "-k village", cardChoices: new Map([["chapel", "excluded"]]),
};
context.tr = (de, en) => context.uiLanguage === "en" ? en : de;
const create = vm.runInNewContext(sourceFunction("createNameConverter") + "\ncreateNameConverter", context);
const engine = create(names);
const name = (id, language = "en") => {
  const row = names.names.find(row => row.id === id);
  assert.ok(row, id);
  return row.titles[language] ?? row.titles.en;
};
const ids = ["cellar", "festival", "gardens", "market", "moat", "remodel", "village", "smithy", "workshop", "chapel"];
for (const language of names.languages) {
  const text = ids.map(id => name(id, language)).join(", ");
  for (const detection of [language, "auto"]) {
    const result = engine.convert(text, detection);
    assert.equal(result.code, "-k " + ids.join(", "));
    assert.equal(result.freeSlots, 0);
  }
  for (let size = 1; size < 10; size++) {
    const result = engine.convert(ids.slice(0, size).map(id => name(id, language)).join(", "), language);
    assert.equal(result.code, "-k " + ids.slice(0, size).join(", "));
    assert.equal(result.freeSlots, 10 - size);
  }
}
assert.equal(engine.convert("Harem", "en").code, "-k farm");
assert.equal(engine.convert("Sauna / Avanto, Encampment / Plunder", "en").code, "-k sauna, encampment");
assert.equal(engine.convert("  king’s   court  ", "en").code, "-k kings_court");
assert.equal(engine.convert("Adventurer", "en").firstEdition[0], "adventurer");
const scenic = [...ids, "advance", "bandit_fort", "academy", "way_of_the_horse"];
assert.equal(engine.convert(scenic.map(id => name(id)).join(", "), "en").code,
  "-k " + scenic.join(", ") + ", -m Landscapes(0,4)");
const partialScenic = engine.convert("Cellar, Advance", "en");
assert.equal(partialScenic.freeSlots, 9);
assert.equal(partialScenic.landscapeCount, 1);
assert.throws(() => engine.convert("Cellar, Cellar", "en"), /mehrfach/);
assert.throws(() => engine.convert("There is no such card", "auto"), /Unbekannte/);
assert.throws(() => engine.convert("Cellar,", "en"), /1–10/);
assert.throws(() => engine.convert("", "auto"), /1–10/);
assert.throws(() => engine.convert("Advance", "en"), /Position/);
assert.throws(() => engine.convert([...ids, "laboratory"].map(id => name(id)).join(", "), "en"), /Position/);
assert.throws(() => engine.convert("a".repeat(8193), "en"), /1–10/);
assert.throws(() => engine.convert("Cellar", "xx"), /Sprache/);
for (const id of ["young_witch", "ferryman", "riverboat", "druid"]) {
  assert.throws(() => engine.convert(name(id), "en"), /Zusatzsetup/);
}
for (const id of ["way_of_the_mouse", "obelisk", "cursed", "league_of_bankers", "approaching_army", "the_rivers_gift"]) {
  assert.throws(() => engine.convert("Cellar, " + name(id), "en"), /Zusatzsetup/);
}
assert.throws(() => engine.convert(["cellar", "advance", "bandit_fort", "academy", "way_of_the_horse", "delve"].map(id => name(id)).join(", "), "en"), /vier Landschaften/);
assert.throws(() => engine.convert("Courtier", "fr"), /mehrdeutig|Mehrdeutiger/);
context.uiLanguage = "en";
assert.throws(() => engine.convert("not a card", "en"), /Unknown names/);
assert.throws(() => engine.convert("Young Witch", "en"), /setup needs/);
assert.throws(() => engine.convert("Cellar, Cellar", "en"), /more than once/);

// Controller tests run the actual listeners and clipboard helper. All fields
// are independent of the generator's output, choices and last result.
class Field {
  constructor() { this.value = ""; this.textContent = ""; this.disabled = false; this.className = ""; this.listeners = new Map(); }
  addEventListener(name, fn) { this.listeners.set(name, fn); }
  focus() { this.focused = true; }
  select() { this.selected = true; }
}
const fields = new Map(["input", "language", "convert", "output", "copy", "status", "copy-status"].map(id => ["name-converter-" + id, new Field()]));
fields.get("name-converter-language").value = "auto";
fields.set("web-card-names", {textContent: JSON.stringify(names)});
let clipboard;
context.document = {getElementById: id => fields.get(id)};
context.navigator = {clipboard: {writeText: async value => {clipboard = value;}}};
vm.runInNewContext(["createNameConverter", "setupNameConverter", "copyText"].map(sourceFunction).join("\n") + "\nsetupNameConverter();", context);
const field = id => fields.get("name-converter-" + id);
assert.equal(field("convert").disabled, true);
const originalResult = context.currentResult;
field("input").value = ids.map(id => name(id)).join(", ");
field("input").listeners.get("input")();
assert.equal(field("convert").disabled, false);
field("convert").listeners.get("click")();
const originalCode = field("output").value;
assert.equal(originalCode, "-k " + ids.join(", "));
assert.equal(field("copy").disabled, false);
context.uiLanguage = "de";
context.interfaceLabels.forEach(render => render());
assert.equal(field("output").value, originalCode);
assert.match(field("status").textContent, /Königreichstapel/);
assert.equal(context.currentResult, originalResult);
assert.equal(context.generatorOutput, "-k village");
assert.equal(context.cardChoices.get("chapel"), "excluded");

async function testClipboard() {
  await field("copy").listeners.get("click")();
  assert.equal(clipboard, originalCode);
  context.navigator.clipboard.writeText = async () => {throw new Error("permission denied");};
  await field("copy").listeners.get("click")();
  assert.ok(field("output").focused && field("output").selected);
  assert.match(field("copy-status").textContent, /Strg\+C/);
  field("input").value = "not a real card";
  field("input").listeners.get("input")();
  assert.equal(field("output").value, "");
  assert.ok(field("copy").disabled);
  field("convert").listeners.get("click")();
  assert.equal(field("status").className, "error");
  assert.equal(context.currentResult, originalResult);
  assert.equal(context.generatorOutput, "-k village");
}
assert.match(html, /<details id="name-converter">/);
assert.match(html, /href="https:\/\/github\.com\/Serpens66\/Dominion\/releases" target="_blank" rel="noopener"/);
assert.ok(!html.includes("Dominion-Konverter.html"));
assert.ok(!fs.existsSync(path.join(__dirname, "Dominion-Konverter.html")));
assert.ok(!fs.readFileSync(path.join(__dirname, "../.github/workflows/generator-pages.yml"), "utf8").includes("Dominion-Konverter.html"));
testClipboard().then(() => console.log("Integrierte Namensumwandlung: sieben Sprachen, Teilmengen, Aliasnamen, gemischte Stapel, Landschaften, Fehler, unabhängige Zustände und Kopier-Fallback geprüft."))
  .catch(error => {console.error(error); process.exitCode = 1;});
