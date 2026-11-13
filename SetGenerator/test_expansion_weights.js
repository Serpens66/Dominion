"use strict";
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm"), assert = require("node:assert/strict");
const html = fs.readFileSync(path.join(__dirname, "Dominion-Generator.html"), "utf8");
const source = name => {
  const start = html.indexOf("function " + name + "(");
  assert.ok(start >= 0, name);
  return html.slice(start, html.indexOf("\n}", start) + 2);
};
const names = ["renderExpansionWeights", "syncExpansionWeights", "editExpansionWeight"];
const ids = ["a", "b", "c", "d"];
const context = {
  expansionModes: new Map(ids.map(id => [id, {value:id === "d" ? "off" : "random"}])),
  expansionWeights: new Map(), equalExpansionWeights: {},
  expansionWeightInputs: new Map(ids.map(id => [id, {field:{value:"",disabled:false},wrapper:{hidden:false},chance:{textContent:""}}])),
  uiLanguage:"de", uiLocale:() => "de-DE", tr:(de,en) => de, expansionName:id => id,
};
const api = vm.runInNewContext(names.map(source).join("\n") + "\n({" + names.join(",") + "})", context);
const weights = () => Object.fromEntries(context.expansionWeights);
const input = id => context.expansionWeightInputs.get(id);
const chance = id => {
  const text = input(id).chance.textContent;
  const match = text.match(/([0-9]+(?:[.,][0-9]+)?)/);
  return match ? Number(match[1].replace(",", ".")) : null;
};
api.syncExpansionWeights();
assert.deepEqual(weights(), {a:1,b:1,c:1,d:1}, "inactive expansions also have a retained default weight");
assert.ok(input("d").wrapper.hidden && input("d").field.disabled);
assert.equal(chance("a"), 33.3);
api.editExpansionWeight("a", 5);
api.editExpansionWeight("b", 3);
api.editExpansionWeight("c", 2);
assert.deepEqual(weights(), {a:5,b:3,c:2,d:1});
assert.deepEqual([chance("a"),chance("b"),chance("c")], [50,30,20]);
assert.deepEqual(ids.map(id => input(id).field.value), ["5","3","2","1"]);
const beforeOtherInputs = [input("b").field.value,input("c").field.value];
input("a").field.value = "5.";
api.editExpansionWeight("a", 5, true);
assert.equal(input("a").field.value, "5.", "active text is preserved during typing");
assert.deepEqual([input("b").field.value,input("c").field.value], beforeOtherInputs);
api.renderExpansionWeights();
for (const id of ["a","b","c"]) api.editExpansionWeight(id, 1);
assert.deepEqual([chance("a"),chance("b"),chance("c")], [33.3,33.3,33.3]);
api.editExpansionWeight("a", 0);
assert.deepEqual(weights(), {a:0,b:1,c:1,d:1});
assert.equal(chance("a"), 0);
assert.equal(chance("b"), 50);
api.editExpansionWeight("b", 0); api.editExpansionWeight("c", 0);
for (const id of ["a","b","c"]) assert.ok(!/NaN|Infinity/.test(input(id).chance.textContent));
context.expansionModes.get("a").value = "off";
context.expansionModes.get("b").value = "always";
api.syncExpansionWeights();
assert.deepEqual(weights(), {a:0,b:0,c:0,d:1}, "mode changes never alter any weight");
assert.equal(input("c").field.disabled, false, "a sole active weight remains editable");
assert.equal(context.equalExpansionWeights.disabled, false);
api.editExpansionWeight("c", 0.1);
assert.equal(chance("c"), 100);
context.expansionModes.get("d").value = "random"; api.syncExpansionWeights();
assert.equal(context.expansionWeights.get("d"), 1);
context.expansionModes.get("d").value = "off"; api.syncExpansionWeights();
assert.equal(context.expansionWeights.get("d"), 1);
const previous = weights();
for (const invalid of [-1,1000.1,NaN,Infinity,"5",0.11]) {
  api.editExpansionWeight("c", invalid);
  assert.deepEqual(weights(), previous, "invalid relative weight must not mutate state: " + invalid);
}
api.editExpansionWeight("c",1000);
assert.equal(context.expansionWeights.get("c"),1000);
// Repeated edits and membership changes must preserve every untouched value.
let state = 19479;
const random = n => { state = (Math.imul(state,1664525)+1013904223) >>> 0; return state % n; };
for (let n=0; n<2000; n++) {
  const id = ids[random(ids.length)], prior = weights();
  if (n % 7 === 0) {
    const mode = context.expansionModes.get(id);
    mode.value = mode.value === "random" ? "off" : "random";
    api.syncExpansionWeights();
    assert.deepEqual(weights(),prior);
  } else {
    context.expansionModes.get(id).value = "random"; api.syncExpansionWeights();
    const value = random(10001) / 10;
    api.editExpansionWeight(id,value);
    assert.equal(context.expansionWeights.get(id),value);
    for (const other of ids.filter(other => other !== id)) assert.equal(context.expansionWeights.get(other),prior[other]);
  }
}
console.log("Relative expansion weights: independent edits, displayed chances, retained mode values, zero weights, validation and 2,000 unchanged-neighbor checks passed.");
