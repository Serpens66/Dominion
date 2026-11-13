"use strict";
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm"), assert = require("node:assert/strict");
const html = fs.readFileSync(path.join(__dirname, "Dominion-Generator.html"), "utf8");
const catalog = JSON.parse(html.match(/id="catalog">([\s\S]+?)<\/script>/)[1]);
const cardById = new Map(catalog.cards.map(card => [card.id, card]));
const context = { cardById };
// Extract the actual standalone-page functions; no parallel implementation.
const source = name => {
  const start = html.indexOf("function " + name + "(");
  return html.slice(start, html.indexOf("\n}", start) + 2);
};
const build = vm.runInNewContext(source("buildKingdomCode") + "\nbuildKingdomCode", context);
const main = ["cellar", "chapel", "moat", "village", "workshop", "smithy", "remodel", "market", "mine", "militia"];
const fixture = patch => ({ main, landscapes: [], assignments: [], components: ["horse", "loot", "ruins"],
  material: [], needsSteamCheck: [], withColonies: false, withShelters: false, ...patch });
const exportSet = patch => build(fixture(patch), (patch?.main ?? main).slice(0, 10));
const plain = "-k " + main.join(", ");
assert.equal(exportSet().code, plain + " -m Landscapes(0,0), NoColonies, NoShelters");
assert.equal(exportSet({ withColonies: true, withShelters: true }).code,
  plain + ", colonies, shelters -m Landscapes(0,0)");
assert.equal(exportSet({ landscapes: ["canal", "basilica", "way_of_the_owl"] }).code,
  plain + ", canal, basilica, way_of_the_owl -m Landscapes(3,3), NoColonies, NoShelters");
for (const [id, targets] of [
  ["young_witch", ["vassal"]], ["ferryman", ["vassal"]], ["riverboat", ["witch"]],
  ["way_of_the_mouse", ["vassal"]], ["druid", ["the_moons_gift", "the_suns_gift", "the_mountains_gift"]],
]) {
  const result = exportSet({ main: [id, ...main.slice(1)], assignments: [{ source: id, targets }] });
  assert.ok(result.code.startsWith("-k " + id + "(" + targets.join(", ") + "), "), id);
  assert.equal(result.omitted.length, 0, id);
}
let result = exportSet({ landscapes: ["obelisk", "nearby"], assignments: [
  { source: "obelisk", targets: ["village"] }, { source: "nearby", targets: ["market"] },
] });
assert.ok(result.code.includes("obelisk(village), nearby(market)"));
// Supplemental piles may themselves require an additional setup selection.
result = exportSet({ main: ["young_witch", ...main.slice(1)], assignments: [
  { source: "young_witch", targets: ["ferryman"] }, { source: "ferryman", targets: ["druid"] },
  { source: "druid", targets: ["the_moons_gift", "the_suns_gift", "the_mountains_gift"] },
] });
assert.ok(result.code.includes("young_witch(ferryman(druid(the_moons_gift, the_suns_gift, the_mountains_gift)))"));
result = exportSet({ main: ["underling", "poet", ...main.slice(2)], assignments: [
  { role: "Verbündeter", targets: ["order_of_masons"] }, { role: "Prophezeiung", targets: ["enlightenment"] },
] });
assert.ok(result.code.includes(", order_of_masons, enlightenment -m Landscapes(0,0)"));
assert.equal(result.omitted.length, 0);
result = exportSet({ main: ["poet", ...main.slice(1)], assignments: [
  { role: "Prophezeiung", targets: ["approaching_army"] }, { source: "approaching_army", targets: ["witch"] },
] });
assert.ok(result.code.includes("approaching_army(witch)"));
// Omitted choices must be returned for the visible partial-code warning.
result = exportSet({ main: ["black_market", ...main.slice(1)], assignments: [
  { source: "black_market", role: "Schwarzmarkt-Deck", targets: ["witch", "vassal"] },
] });
assert.ok(result.code.startsWith("-k black_market, "));
assert.equal(result.omitted[0].role, "Schwarzmarkt-Deck");
assert.ok(!result.code.includes("black_market("));
result = exportSet({ landscapes: ["canal", "basilica", "way_of_the_owl", "nearby", "advance"] });
assert.ok(!result.code.includes("advance"));
assert.equal(result.omitted[0].targets[0], "advance");
result = exportSet({ main: ["underling", ...main.slice(1)],
  landscapes: ["canal", "basilica", "way_of_the_owl", "advance"],
  assignments: [{ role: "Verbündeter", targets: ["order_of_masons"] }] });
assert.equal(result.omitted[0].targets[0], "order_of_masons");
// A target that exists only in the >10-pile planning area cannot be imported.
result = exportSet({ main: [...main, "witch"], landscapes: ["obelisk"],
  assignments: [{ source: "obelisk", targets: ["witch"] }] });
assert.ok(!result.code.includes("witch"));
assert.equal(result.omitted[0].source, "obelisk");
// Use the actual renderer to verify labels and warning visibility after redraw.
Object.assign(context, {
  standardKingdomSize: 10, uiLanguage: "de", clearOutputs() {},
  tr: (de, en) => context.uiLanguage === "en" ? en : de,
  cardTitle: (card, language) => card.text[language][0], uiLocale: () => "de",
  output: {}, effects: {}, copy: {}, exportWarning: {}, status: {}, randomExpansionResult: {},
  planningResult: {}, planningCards: {}, expansions: [], formatDoublePiles: () => "double",
  setupTerm: value => value, showCards() {}, showSetup() {}, updateEffectSet() {},
  document: { getElementById: () => context.label }, label: {},
});
const show = vm.runInNewContext(source("buildKingdomCode") + "\n" + source("showDraw") + "\nshowDraw", context);
show(fixture({ landscapes: ["canal"], needsSteamCheck: ["Landschaften"] }));
assert.ok(context.output.value.includes(", canal -m Landscapes(1,1)"));
assert.equal(context.label.textContent, "Kingdom-Code");
assert.equal(context.copy.textContent, "Für Dominion kopieren");
assert.match(context.exportWarning.textContent, /Importtest/);
show(fixture({ main: ["black_market", ...main.slice(1)], assignments: [
  { source: "black_market", role: "Schwarzmarkt-Deck", targets: ["witch"] },
] }));
assert.equal(context.label.textContent, "Steam-Teilcode");
assert.match(context.exportWarning.textContent, /Schwarzmarkt-Deck/);
assert.ok(context.exportWarning.textContent.includes(cardById.get("witch").text.de[0]));
context.uiLanguage = "en";
show(fixture());
assert.equal(context.label.textContent, "Kingdom code");
assert.equal(context.copy.textContent, "Copy into Dominion");
assert.equal(context.exportWarning.hidden, true);
console.log("Kingdom export: setup options, Landscapes, nested assignments, Allies/Prophecies, capacity limits and renderer warnings passed.");
