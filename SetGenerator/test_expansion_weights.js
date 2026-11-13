"use strict";
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm"), assert = require("node:assert/strict");
const html = fs.readFileSync(path.join(__dirname, "Dominion-Generator.html"), "utf8");
const source = name => {
  const start = html.indexOf("function " + name + "(");
  return html.slice(start, html.indexOf("\n}", start) + 2);
};
const names = ["allocateExpansionWeights", "renderExpansionWeights", "syncExpansionWeights", "editExpansionWeight"];
const context = {
  expansionModes: new Map(["a", "b", "c", "d"].map(id => [id, {value:"random"}])),
  expansionWeights: new Map(), expansionWeightInputs: new Map(), equalExpansionWeights: {},
};
const api = vm.runInNewContext(names.map(source).join("\n") + "\n({" + names.join(",") + "})", context);
const weights = () => Object.fromEntries(context.expansionWeights);
const total = () => [...context.expansionWeights.values()].reduce((a,b) => a+b, 0);
api.syncExpansionWeights();
assert.deepEqual(weights(), {a:250,b:250,c:250,d:250});
api.editExpansionWeight("a", 700);
assert.deepEqual(weights(), {a:700,b:100,c:100,d:100});
api.editExpansionWeight("a", 1000);
assert.deepEqual(weights(), {a:1000,b:0,c:0,d:0});
api.editExpansionWeight("a", 699);
assert.deepEqual(weights(), {a:699,b:101,c:100,d:100});
context.expansionModes.get("b").value = "off"; api.syncExpansionWeights();
assert.equal(context.expansionWeights.has("b"), false);
assert.equal(total(), 1000);
assert.ok(Math.abs(context.expansionWeights.get("a") / context.expansionWeights.get("c") - 6.99) < .05);
context.expansionModes.get("b").value = "random"; api.syncExpansionWeights();
assert.equal(context.expansionWeights.get("b"), 250);
assert.equal(total(), 1000);
api.syncExpansionWeights({a:999,b:1,c:0,d:0});
assert.deepEqual(weights(), {a:999,b:1,c:0,d:0});
// A zero-weight survivor receives the full share when the only positive entry leaves.
context.expansionModes.get("a").value = "off";
context.expansionModes.get("b").value = "off"; api.syncExpansionWeights();
assert.deepEqual(weights(), {c:500,d:500});
context.expansionModes.get("d").value = "off"; api.syncExpansionWeights();
assert.deepEqual(weights(), {c:1000});
assert.equal(context.equalExpansionWeights.disabled, true);
context.expansionModes.get("c").value = "off"; api.syncExpansionWeights();
assert.deepEqual(weights(), {});
for (const mode of context.expansionModes.values()) mode.value = "random";
api.syncExpansionWeights();
// Repeated edits and mode transitions must never accumulate rounding drift.
let state = 19479;
const random = n => { state = (Math.imul(state,1664525)+1013904223) >>> 0; return state % n; };
const ids = [...context.expansionModes.keys()];
for (let n=0; n<2000; n++) {
  const id = ids[random(ids.length)];
  if (n % 7 === 0) {
    const mode = context.expansionModes.get(id);
    mode.value = mode.value === "random" ? "off" : "random";
    api.syncExpansionWeights();
  } else api.editExpansionWeight(id, random(1001));
  assert.equal(total(), context.expansionWeights.size ? 1000 : 0);
  for (const [key,value] of context.expansionWeights) {
    assert.equal(context.expansionModes.get(key).value, "random");
    assert.ok(Number.isInteger(value) && value >= 0 && value <= 1000);
  }
}
console.log("Expansion percentages: proportional edits, largest remainders, zero shares, mode transitions and 2,000 edits without rounding drift passed.");
