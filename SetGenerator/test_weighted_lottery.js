"use strict";
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm"), assert = require("node:assert/strict");
const worker = fs.readFileSync(path.join(__dirname, "generator_solver.js"), "utf8");
const html = fs.readFileSync(path.join(__dirname, "Dominion-Generator.html"), "utf8");
const catalog = JSON.parse(html.match(/id="catalog">([\s\S]+?)<\/script>/)[1]);
const byId = new Map(catalog.cards.map(card => [card.id, card]));
const api = vm.runInNewContext(worker + "\n({expansionCombinationProbabilities, weightedExpansionOrder, analyzeExpansionLottery, timedOut})");
const near = (value, expected) => assert.ok(Math.abs(value - expected) < 1e-12, `${value} != ${expected}`);
const probabilities = (ids, count, weights) => api.expansionCombinationProbabilities(ids, count, weights);
const p = probabilities(["a", "b", "c"], 1, {a:500,b:250,c:250});
assert.deepEqual(Array.from(p, entry => entry.ids.join()), ["a", "b", "c"]);
p.forEach((entry, i) => near(entry.probability, [0.5, 0.25, 0.25][i]));
const pairs = probabilities(["a", "b", "c"], 2, {a:500,b:250,c:250});
assert.deepEqual(Array.from(pairs, entry => entry.ids.join()), ["a,b", "a,c", "b,c"]);
pairs.forEach((entry, i) => near(entry.probability, [5/12,5/12,1/6][i]));
near(pairs.reduce((sum, entry) => sum + entry.probability, 0), 1);
// Raw relative and decimal weights preserve the same sampling distribution.
for (const weights of [{a:5,b:2.5,c:2.5}, {a:.2,b:.1,c:.1}]) {
  probabilities(["a", "b", "c"], 2, weights).forEach((entry, i) => near(entry.probability, pairs[i].probability));
}
const diverse = probabilities(["a", "b", "c", "d"], 3, {a:1,b:3,c:11,d:29});
near(diverse.reduce((sum, entry) => sum + entry.probability, 0), 1);
const equal = probabilities(["a", "b", "c", "d"], 2);
equal.forEach(entry => near(entry.probability, 1/6));
assert.deepEqual(Array.from(probabilities(["a", "b"], 1, {a:0,b:1000}), entry => entry.ids.join()), ["b"]);
assert.equal(probabilities(["a", "b"], 2, {a:0,b:1000}).length, 0);
assert.equal(probabilities(["a", "b"], 0, {a:0,b:0})[0].probability, 1);
assert.equal(probabilities(["a", "b"], 2, {a:1,b:99})[0].probability, 1);
for (const value of [-1, NaN, Infinity]) assert.throws(() => probabilities(["a"], 1, {a:value}), /Invalid expansion lottery weight/);
assert.throws(() => api.expansionCombinationProbabilities(["a"], 1, {a:1}, Date.now()-1), error => error === api.timedOut);
// Interrupt an enumeration in progress, not only the initial deadline check.
let now = 0;
const clockApi = vm.runInNewContext(worker + "\n({expansionCombinationProbabilities, timedOut})", {Date:class extends Date {static now(){return now++;}}});
assert.throws(() => clockApi.expansionCombinationProbabilities(Array.from({length:18}, (_,i)=>String(i)), 9, undefined, 3), error => error === clockApi.timedOut);
assert.ok(now >= 4);
const controlled = values => {
  const math = Object.create(Math); let at = 0;
  math.random = () => values[at++] ?? .999999;
  return vm.runInNewContext(worker + "\n({weightedExpansionOrder,analyzeExpansionLottery})", {Math:math});
};
let order = controlled([.9,.1,.5]).weightedExpansionOrder(pairs);
assert.equal(order[0].join(), "a,c");
assert.equal(new Set(Array.from(order, entry => entry.join())).size, 3);
// Exponential clocks yield the same weighted conditional distribution after
// deleting impossible candidates. Compare every deterministic first rank here.
order = controlled([.8,.2,.7]).weightedExpansionOrder([
  {ids:["impossible"],probability:.9},{ids:["b"],probability:.07},{ids:["c"],probability:.03},
]);
assert.equal(order.filter(entry=>entry[0]!=="impossible")[0][0], "b");
const base = catalog.cards.filter(card=>card.kingdomPile && card.expansion==="base" && card.edition!=="first").slice(0,9);
const candidates = [byId.get("raze"),byId.get("supplies")];
const optional = candidates.map(card=>card.expansion);
const request = {
  fixed:base,candidates,extras:[...base,...candidates],components:catalog.cards.filter(card=>!card.kingdomPile),rules:[],
  landscapeCount:{min:0,max:0},landscapePool:[],landscapeRules:[],requireLoot:false,colonies:"never",shelters:"never",
  expansionLottery:{always:["base"],required:[],optional,count:1,autoLandscapes:[],weights:{adventures:900,menagerie:100}},
};
const draw = (input=request) => controlled([.5,.5]).analyzeExpansionLottery(input,()=>{});
let out = draw();
assert.equal(out.result.randomExpansions.join(), "adventures");
assert.ok(out.result.main.includes("raze"));
// The heavier impossible combination is discarded; the positive feasible set
// is still found without promoting an excluded zero-weight set.
out = draw({...request,rules:[{ids:["supplies"],min:1,max:1}]});
assert.equal(out.result.randomExpansions.join(), "menagerie");
out = draw({...request,rules:[{ids:["supplies"],min:1,max:1}],expansionLottery:{...request.expansionLottery,weights:{adventures:1000,menagerie:0}}});
assert.equal(out.total,0n); assert.equal(out.result,null);
out = draw({...request,expansionLottery:{...request.expansionLottery,count:2,weights:{adventures:1000,menagerie:0}}});
assert.equal(out.total,0n);
out = draw({...request,expansionLottery:{...request.expansionLottery,required:["menagerie"],optional:["adventures"],count:1,weights:{adventures:1000,menagerie:0}}});
assert.equal(out.result.randomExpansions.join(), "menagerie");
// Old callers keep the old Fisher-Yates order when weights are absent.
const legacy = {...request,expansionLottery:{...request.expansionLottery}};
delete legacy.expansionLottery.weights;
assert.equal(controlled([.999999]).analyzeExpansionLottery(legacy,()=>{}).result.randomExpansions.join(), "adventures");
assert.equal(api.analyzeExpansionLottery(request,()=>{},Date.now()-1).total,null);
console.log("Weighted lottery: exact subset probabilities, weighted ordering, required/zero weights, infeasible pools, deadlines and legacy callers passed.");
