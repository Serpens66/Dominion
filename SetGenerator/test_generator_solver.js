const fs=require("node:fs"), assert=require("node:assert/strict"),vm=require("node:vm");
const path=require("node:path");
const { createHash } = require("node:crypto");
const html=fs.readFileSync(path.join(__dirname,"Dominion-Generator.html"),"utf8");
const worker=fs.readFileSync(path.join(__dirname,"generator_solver.js"),"utf8");
const readScript = (page, id) => page.match(new RegExp('id="' + id + '">([\\s\\S]+?)</script>'))?.[1];
const normalize = source => source.replace(/\r\n/g, "\n").trim();
assert.equal(normalize(readScript(html, "generator-solver")), normalize(worker), "embedded worker must match the solver source");
assert.equal(readScript(html, "embedded-image-pack"), undefined, "preview images are stored in separate files");
const api=vm.runInNewContext(worker+"\n({analyzeKingdom,assignSetup,chooseLandscapes})");
const catalog=JSON.parse(html.match(/id="catalog">([\s\S]+?)<\/script>/)[1]);
const all=catalog.cards, byId=new Map(all.map(c=>[c.id,c])), get=(...ids)=>ids.map(id=>{assert.ok(byId.has(id),id);return byId.get(id);});
const imagePath = path.join(__dirname, "Dominion-Kartenbilder.json");
if (fs.existsSync(imagePath)) {
  const pack = JSON.parse(fs.readFileSync(imagePath, "utf8").replace(/^\uFEFF/, ""));
  assert.equal(pack.format, "dominion-card-images");
  assert.equal(pack.version, 1);
  for (const images of [pack.images, ...Object.values(pack.cards ?? {})]) {
    assert.ok(Object.keys(images).length > 0);
    for (const [id, source] of Object.entries(images)) {
      assert.ok(byId.has(id), id);
      assert.match(source, /^data:image\/(?:png|webp);base64,/);
    }
  }
}

const base=all.filter(c=>c.kingdomPile&&c.expansion==="base"&&c.edition!=="first");
const intrigue=all.filter(c=>c.kingdomPile&&c.expansion==="intrigue"&&c.edition!=="first");
const allPiles=all.filter(c=>c.kingdomPile);
const components=all.filter(c=>!c.kingdomPile);
assert.equal(all.length,853);
assert.deepEqual(catalog.nativeIds, {
  gameBuild: 3315, steamBuild: 17683350, rulesVersion: 23,
  exeSha256: "56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877",
});
assert.ok(all.every(card => Number.isInteger(card.nativeId) && card.nativeId >= 0x100 && card.nativeId < 0x1400));
assert.equal(new Set(all.map(card => card.nativeId)).size, 853);
for (const [id, number] of Object.entries({cellar: 512, village: 518, farm: 792, knights: 2332, sauna: 4869, way_of_the_mouse: 3898, foray: 4412})) {
  assert.equal(byId.get(id).nativeId, number, "native profile ID for " + id);
}
assert.equal(allPiles.length,498);
assert.equal(base.length,26);
const imageFiles = JSON.parse(readScript(html, "card-image-files"));
assert.equal(Object.keys(imageFiles.images).length, 845);
assert.ok(allPiles.every(card => imageFiles.images[card.id]), "every Kingdom pile has an illustration");
const entriesWithoutArt = ["black_market_deck", "boons", "exile", "hexes", "loot_pile", "prizes", "rewards", "shuffling"];
assert.deepEqual(all.filter(card => !imageFiles.images[card.id]).map(card => card.id).sort(), entriesWithoutArt);
for (const [id, value] of Object.entries({
  copper: ["coin", "1"], silver: ["coin", "2"], gold: ["coin", "3"], platinum: ["coin", "5"],
  colony: ["vp", "10"], curse: ["vp", "−1"], potion: ["potion", ""],
})) {
  assert.ok(imageFiles.images[id], "illustration for " + id);
  assert.deepEqual(byId.get(id).printedValue, value, "printed value for " + id);
}
for (const images of [imageFiles.images, ...Object.values(imageFiles.cards)]) {
  for (const [id, filename] of Object.entries(images)) {
    assert.ok(byId.has(id), id);
    assert.match(filename, /^card-images\/(illustrations|de|en)\/[a-z0-9_]+-[a-f0-9]{12}\.(png|webp)$/);
    const bytes = fs.readFileSync(path.join(__dirname, filename));
    const digest = createHash("sha256").update(bytes).digest("hex").slice(0, 12);
    assert.ok(filename.endsWith("-" + digest + path.extname(filename)), "image content matches its filename");
    if (filename.endsWith(".png")) assert.equal(bytes.subarray(0, 8).toString("hex"), "89504e470d0a1a0a");
    else {
      assert.equal(bytes.subarray(0, 4).toString(), "RIFF");
      assert.equal(bytes.subarray(8, 12).toString(), "WEBP");
    }
  }
}
for (const language of ["de", "en"]) {
  assert.equal(Object.keys(imageFiles.cards[language]).length, 26);
  assert.ok(base.every(card => imageFiles.cards[language][card.id]), "complete Base cards in " + language);
}
assert.ok(all.every(c=>c.text.de[0]&&c.text.en[0]&&c.related.every(id=>byId.has(id))));
assert.ok(!byId.has("harem")&&byId.has("farm"));
const request=(fixed,candidates,updates={})=>({
  fixed,candidates,extras:[...fixed,...candidates],components,rules:[],landscapeCount:{min:0,max:0},
  landscapePool:[],landscapeRules:[],requireLoot:false,colonies:"never",shelters:"never",...updates,
});
const rule=(cards,min,max,extra={})=>({ids:cards.map(c=>c.id),min,max,...extra});
function choose(n,k){let value=1n;for(let i=1;i<=k;i++)value=value*BigInt(n-i+1)/BigInt(i);return value;}
let out=api.analyzeKingdom(request([],base),()=>{});
assert.equal(out.total,choose(26,10));
assert.equal(out.result.main.length,10);
for(let i=0;i<10;i++)assert.equal(new Set(out.drawExact().main).size,10);
for(const kingdomSize of [11,12,13]){
 const pinned=base.slice(0,2);
 const available=base.slice(2,15);
 const quota=rule(pinned,2,2);
 out=api.analyzeKingdom(request(pinned,available,{kingdomSize,rules:[quota]}),()=>{});
 assert.equal(out.total,choose(available.length,kingdomSize-pinned.length));
 assert.equal(out.result.main.length,kingdomSize);
 assert.ok(pinned.every(card=>out.result.main.includes(card.id)));
 assert.equal(api.analyzeKingdom(request([],base.slice(0,kingdomSize-1),{kingdomSize}),()=>{}).total,0n);
}

out=api.analyzeKingdom(request([],base.concat(intrigue),{rules:[rule(base,6,6),rule(intrigue,4,4)]}),()=>{});
assert.equal(out.total,choose(base.length,6)*choose(intrigue.length,4));
assert.equal(out.result.main.filter(id=>byId.get(id).expansion==="intrigue").length,4);
const fixed=base.filter(c=>!c.types.includes("attack")&&c.id!=="moat").slice(0,8);
const candidates=get("moat","witch","village","cellar","militia","sentry").filter(c=>!fixed.includes(c));
const conditional=rule(get("moat"),1,1,{whenAttacked:true});
out=api.analyzeKingdom(request(fixed,candidates,{rules:[conditional]}),()=>{});
let n=0;
for(let i=0;i<candidates.length;i++)for(let j=i+1;j<candidates.length;j++){
 const pair=[candidates[i],candidates[j]];
 if(!pair.some(c=>c.types.includes("attack"))||pair.some(c=>c.id==="moat"))n++;
}
assert.equal(out.total,BigInt(n));
const main=get("young_witch","ferryman","riverboat","druid","underling","imperial_envoy","witch","market","moat","chapel");
const extras=[...main,...get("village","shop","laboratory","smithy","militia","cellar")];
out=api.analyzeKingdom(request(main,[],{extras,requireLoot:true,landscapePool:get("foray","obelisk","cursed"),
  landscapeCount:{min:3,max:3}}),()=>{});
assert.equal(out.total,1n);
assert.equal(out.result.landscapes.length,3);
for(const id of ["young_witch","ferryman","riverboat","druid","obelisk","cursed"])assert.ok(out.result.assignments.some(a=>a.source===id),id);
assert.ok(out.result.assignments.some(a=>a.role==="Verbündeter"));
assert.ok(out.result.components.some(id=>byId.get(id).types.includes("loot")));
assert.ok(out.result.needsSteamCheck.includes("Landschaften"));
const missing=api.analyzeKingdom(request(main,[],{extras:main}),()=>{});
assert.equal(missing.total,0n);
const omens=get("mountain_shrine");
const prophecyMain=[...base.slice(0,9),...omens];
out=api.analyzeKingdom(request(prophecyMain,[],{extras:[...prophecyMain,...get("witch")],
  components:get("approaching_army")}),()=>{});
assert.equal(out.total,1n);
assert.ok(out.result.assignments.some(a=>a.source==="approaching_army" && a.targets[0]==="witch"));
const distinct=rule(intrigue,1,1,{distinct:true});
out=api.analyzeKingdom(request(fixed,get("moat","courtyard","pawn","witch"),{rules:[distinct]}),()=>{});
assert.equal(out.total,5n);
const before=Date.now();
out=api.analyzeKingdom(request([],allPiles),()=>{},Date.now()+240);
assert.ok(Date.now()-before<2000);
assert.ok(out.result && out.result.main.length===10 && out.total===null);

const landscapeOptions=get("foray","advance","obelisk");
const landscapeConstraint=rule(get("foray"),1,1);
const scenic=api.analyzeKingdom(request(base.slice(0,10),[],{
  landscapePool:landscapeOptions, landscapeCount:{min:2,max:2}, requireLoot:true,
  landscapeRules:[landscapeConstraint,rule(landscapeOptions,2,2,{distinct:true})],
}),()=>{});
assert.equal(scenic.total,1n);
assert.ok(scenic.result.landscapes.includes("foray"));
assert.equal(new Set(scenic.result.landscapes.map(id=>byId.get(id).expansion)).size,2);
// Keep combinations in their original order to exercise the retry after an impossible pool.
const lotteryMath = Object.create(Math);
lotteryMath.random = () => 0.999999;
const drawLottery = vm.runInNewContext(worker + "\nanalyzeExpansionLottery", { Math: lotteryMath });
const foray = byId.get("foray");
const advance = byId.get("advance");
const always = ["base", "intrigue", "prosperity"];
const optional = [advance.expansion, foray.expansion];
const lottery = request(base.slice(0, 10), [], {
  landscapePool: [advance, foray],
  landscapeCount: { min: 1, max: 1 },
  landscapeRules: [rule([foray], 1, 1)],
  expansionLottery: { always, required: [], optional, count: 1, autoLandscapes: optional },
});
out = drawLottery(lottery, () => {});
assert.equal(out.total, null);
assert.equal(out.result.randomExpansions.join(), foray.expansion);
assert.equal(out.result.landscapes.join(), foray.id);
assert.ok(out.result.main.every(id => byId.get(id).expansion === "base"));

const explicit = {
  ...lottery,
  expansionLottery: { ...lottery.expansionLottery, autoLandscapes: [advance.expansion] },
};
out = drawLottery(explicit, () => {});
assert.equal(out.result.randomExpansions.join(), advance.expansion);
assert.equal(out.result.landscapes.join(), foray.id);
assert.equal(drawLottery(lottery, () => {}, Date.now() - 1).total, null);
assert.equal(drawLottery({
  ...lottery, expansionLottery: { ...lottery.expansionLottery, required: optional, optional: [] },
}, () => {}).total, 0n);

const randomGroups = [
  get("raze", "ranger", "magpie"),
  get("supplies", "stockpile", "village_green"),
  get("monastery", "conclave", "devils_workshop"),
  get("church", "sauna", "dismantle"),
  get("acting_troupe", "research", "cargo_ship"),
];
const randomSets = randomGroups.map(group => group[0].expansion);
assert.equal(new Set(randomSets).size, 5);
const randomQuotas = (min, max) => randomGroups.map(group =>
  rule(group, min, max, { conditionalExpansion: group[0].expansion }));
const randomRequest = request(base.slice(0, 8), randomGroups.flat(), {
  rules: randomQuotas(1, 3),
  expansionLottery: { always: ["base"], required: [], optional: randomSets, count: 2, autoLandscapes: [] },
});
function assertRandomQuotas(result, min, max) {
  assert.ok(result);
  assert.equal(result.main.length, 10);
  assert.equal(new Set(result.randomExpansions).size, 2);
  for (const expansion of randomSets) {
    const count = result.main.filter(id => byId.get(id).expansion === expansion).length;
    if (result.randomExpansions.includes(expansion)) assert.ok(count >= min && count <= max);
    else assert.equal(count, 0);
  }
}
out = drawLottery(randomRequest, result => assertRandomQuotas(result, 1, 3));
assertRandomQuotas(out.result, 1, 3);

// Only the two selected minimums apply, even when all five would exceed the Kingdom size.
out = drawLottery({
  ...randomRequest, fixed: base.slice(0, 4), rules: randomQuotas(3, 3),
}, result => assertRandomQuotas(result, 3, 3));
assertRandomQuotas(out.result, 3, 3);
assert.equal(drawLottery({
  ...randomRequest, fixed: base.slice(0, 4), rules: randomQuotas(6, 6),
}, () => {}).total, 0n);

// An unavailable quota rules out that candidate, while another candidate remains usable.
const menagerie = get("supplies", "stockpile", "village_green", "paddock", "barge", "coven");
out = drawLottery(request(base.slice(0, 4), [...randomGroups[0], ...menagerie], {
  rules: [rule(randomGroups[0], 6, 6, { conditionalExpansion: "adventures" }),
    rule(menagerie, 6, 6, { conditionalExpansion: "menagerie" })],
  expansionLottery: { always: ["base"], required: [], optional: ["adventures", "menagerie"], count: 1, autoLandscapes: [] },
}), () => {});
assert.equal(out.result.randomExpansions.join(), "menagerie");
assert.equal(out.result.main.filter(id => byId.get(id).expansion === "menagerie").length, 6);

out = drawLottery({
  ...randomRequest,
  fixed: [...base.slice(0, 7), randomGroups[2][0]],
  candidates: randomGroups.flat().filter(card => card !== randomGroups[2][0]),
  expansionLottery: { ...randomRequest.expansionLottery, required: ["nocturne"],
    optional: randomSets.filter(id => id !== "nocturne") },
}, () => {});
assertRandomQuotas(out.result, 1, 3);
assert.ok(out.result.randomExpansions.includes("nocturne"));
assert.ok(out.result.main.includes("monastery"));

out = drawLottery({
  ...randomRequest, fixed: base.slice(0, 10), candidates: [],
  expansionLottery: { ...randomRequest.expansionLottery, count: 0 },
}, () => {});
assert.ok(out.result);
assert.equal(out.result.randomExpansions.length, 0);
assert.ok(out.result.main.every(id => byId.get(id).expansion === "base"));

// Run the actual frontend predicates, rather than a second classification copy.
const propertyContext = {
  catalog, allCards: all, cards: allPiles, cardById: byId, standardKingdomSize: 10,
  landscapeLimit: 4, kingdomSize: 10, includeRelated: { checked: true }, poolChoices: new Map(),
  pinnedPriority: {checked: false},
  expansions: Object.keys(catalog.expansions), cardTypeIcons: {}, resourceIcons: {},
};
const predicateSource = html.slice(html.indexOf("const landscapeTypes ="), html.indexOf("function makeGameIcon("));
const solverCardSource = html.match(/function solverCard\(card\) \{[\s\S]*?\n\}/)[0];
const rangeValidationSource = html.match(/function validRangePair\([\s\S]*?\n\}/)[0];
const propertyAPI = vm.runInNewContext(predicateSource + "\n" + solverCardSource + "\n" +
  "const isValidRangeBound = (v, max) => Number.isInteger(v) && v >= 0 && v <= max;\n" +
  rangeValidationSource + "\n({countRules,propertyFamily,solverCard,rangeLimit,validRangePair})", propertyContext);
const propertyRules = new Map(propertyAPI.countRules.map(rule => [rule.id, rule]));
const matchesProperty = (id, card) => propertyRules.get(id).matches(byId.get(card));
assert.ok(matchesProperty("attack", "peasant"));
assert.ok(matchesProperty("cards", "peasant"));
assert.ok(matchesProperty("attack", "page"));
assert.ok(matchesProperty("defense", "page"));
assert.ok(matchesProperty("trash", "pixie")); // Goat and the Flame's Gift.
assert.ok(matchesProperty("trash", "vampire")); // Bat; its backlink must not loop.
assert.ok(matchesProperty("cards", "foray")); // The Loot deck includes Figurehead.
assert.ok(matchesProperty("attack", "foray")); // Sword.
assert.ok(matchesProperty("defense", "foray")); // Shield.
assert.ok(!matchesProperty("non-action", "foray")); // Spell Scroll is an Action.
assert.ok(!matchesProperty("non-action", "pixie"));
assert.ok(matchesProperty("non-action", "flag"));
assert.ok(!matchesProperty("attack", "way_of_the_mouse"));
assert.equal(byId.get("way_of_the_mouse").randomSetup, "mouse");
assert.ok(byId.get("way_of_the_mouse").propertyRelated.length === 0);
assert.ok(byId.get("druid").propertyRelated.every(id => !byId.get(id).types.includes("boon")));
assert.ok(matchesProperty("cost-2", "peasant")); // Own cost remains 2, not Soldier's 3.
assert.ok(!matchesProperty("cost-3", "peasant"));
assert.ok(!matchesProperty("exp-prosperity", "peasant"));
for (const card of all) {
  assert.ok(card.related.every(id => byId.has(id)), card.id);
  assert.equal(new Set(card.propertyRelated).size, card.propertyRelated.length, card.id);
  assert.ok(!card.propertyRelated.includes(card.id), card.id);
  assert.equal(new Set(card.relatedGroups.flatMap(group => group.ids)).size, card.related.length, card.id);
}
assert.deepEqual(byId.get("soldier").relatedGroups[0].ids, ["fugitive", "disciple", "teacher"]);
assert.ok(byId.get("treasurer").propertyRelated.includes("key"));
assert.ok(byId.get("the_swamps_gift").propertyRelated.includes("will_o_wisp"));
assert.ok(byId.get("the_flames_gift").nativeFilters.includes("Trashing"));
assert.ok(byId.get("zombie_apprentice").nativeFilters.includes("Trashing"));
assert.ok(byId.get("fugitive").nativeFilters.includes("ExtraCards"));
assert.ok(byId.get("goat").nativeFilters.includes("Trashing"));

const sourceRule = (id, min, max) => {
  const source = propertyRules.get(id);
  return rule(allPiles.filter(source.matches), min, max, {
    property: true, whenAttacked: source.whenAttacked,
    landscapeIds: all.filter(card => isLandscapeCard(card) && source.matches(card)).map(card => card.id),
  });
};
function isLandscapeCard(card) { return ["event", "landmark", "project", "way", "trait"].some(type => card.types.includes(type)); }
const relatedRequest = (fixed, candidates, updates = {}) => {
  const req = request(fixed, candidates, { ...updates, includeRelated: propertyContext.includeRelated.checked });
  for (const key of ["fixed", "candidates", "extras", "components", "landscapePool"]) req[key] = req[key].map(propertyAPI.solverCard);
  return req;
};
const filler = get("cellar", "chapel", "merchant", "mine", "remodel", "workshop", "vassal", "harbinger", "poacher");
const travellerMain = [...filler, ...get("peasant")];
out = api.analyzeKingdom(relatedRequest(travellerMain, [], { rules: [sourceRule("attack", 0, 0)] }), () => {});
assert.equal(out.total, 0n);
out = api.analyzeKingdom(relatedRequest(travellerMain, [], { rules: [sourceRule("cards", 1, 1)] }), () => {});
assert.equal(out.total, 1n, "Fugitive makes one matching source, not a separate pile");
out = api.analyzeKingdom(relatedRequest(travellerMain, [], { rules: [sourceRule("cards", 2, 2)] }), () => {});
assert.equal(out.total, 0n);
out = api.analyzeKingdom(relatedRequest(travellerMain, [], { rules: [sourceRule("defense", 1, 2)] }), () => {});
assert.equal(out.total, 0n, "Soldier activates the defense quota");
out = api.analyzeKingdom(relatedRequest(travellerMain, [], { rules: [sourceRule("attack-reaction", 1, 2)] }), () => {});
assert.equal(out.total, 0n, "Soldier activates the Attack Reaction quota");

const plainMain = [...filler, ...get("village")];
out = api.analyzeKingdom(relatedRequest(plainMain, [], {
  landscapePool: get("demand"), landscapeCount: {min: 1, max: 1}, rules: [sourceRule("cards", 1, 1)],
}), () => {});
assert.equal(out.total, 1n, "a fixed Landscape extra can satisfy a property minimum");
out = api.analyzeKingdom(relatedRequest(plainMain, [], {
  landscapePool: get("demand"), landscapeCount: {min: 1, max: 1}, rules: [sourceRule("cards", 0, 0)],
}), () => {});
assert.equal(out.total, 0n, "a fixed Landscape extra is also checked against the maximum");
out = api.analyzeKingdom(relatedRequest(plainMain, [], {
  landscapePool: get("foray"), landscapeCount: {min: 1, max: 1}, rules: [sourceRule("defense", 0, 0)],
}), () => {});
assert.equal(out.total, 0n, "Loot's Sword activates the conditional quota and Shield matches it");
out = api.analyzeKingdom(relatedRequest(plainMain, [], {
  landscapePool: get("way_of_the_mouse"), landscapeCount: {min: 1, max: 1},
  extras: [...plainMain, ...get("militia", "smithy", "witch", "moat")], rules: [sourceRule("cards", 0, 0), sourceRule("attack", 0, 0)],
}), () => {});
assert.equal(out.total, 1n, "uncontrolled random setup targets do not affect property quotas");
assert.deepEqual(Array.from(out.result.ignoredRandomSources), ["way_of_the_mouse"]);

// Compare full enumeration of a small pool with the source-based solver counts.
const smallPool = get("peasant", "page", "village", "smithy", "moat", "urchin");
const smallRule = sourceRule("attack", 1, 1);
let exhaustive = 0n;
for (let a=0; a<smallPool.length; a++) for (let b=a+1; b<smallPool.length; b++) {
  if ([smallPool[a],smallPool[b]].filter(card => propertyRules.get("attack").matches(card)).length === 1) exhaustive++;
}
out = api.analyzeKingdom(relatedRequest([], smallPool, {kingdomSize: 2, rules: [smallRule]}), () => {});
assert.equal(out.total, exhaustive);
const withVampire = [...filler, ...get("vampire")];
out = api.analyzeKingdom(relatedRequest(withVampire, []), () => {});
assert.equal(out.total, 1n);
assert.ok(out.result.components.includes("bat"));
assert.ok(!out.result.components.includes("vampire"), "a backlink must not add the main pile a second time");
propertyContext.includeRelated.checked = false;
assert.ok(!matchesProperty("attack", "peasant"));
assert.ok(!matchesProperty("cards", "peasant"));
out = api.analyzeKingdom(relatedRequest(travellerMain, [], { rules: [sourceRule("attack", 0, 0)] }), () => {});
assert.equal(out.total, 1n, "default mode retains the existing primary-pile behavior");
assert.equal(propertyAPI.validRangePair(propertyRules.get("attack"), ["0", "14"], 10, true), true);
assert.equal(propertyAPI.validRangePair(propertyRules.get("attack"), ["0", "14"], 10, false), false);
assert.equal(propertyAPI.validRangePair(propertyRules.get("cost-2"), ["0", "14"], 10, true), false);

const sourceFunction = name => html.match(new RegExp("(?:async )?function " + name + "\\([\\s\\S]*?\\n\\}"))[0];
// Pinned sources must explain proven upper-bound conflicts before a Worker starts.
const pinnedContext = {
  pinnedPriority: {checked: false},
  countRules: propertyAPI.countRules, includeRelated: propertyContext.includeRelated,
  uiLanguage: "de", cardTitle: (card, language) => card.text[language][0],
};
pinnedContext.tr = (de, en) => pinnedContext.uiLanguage === "en" ? en : de;
pinnedContext.ruleLabel = rule => pinnedContext.uiLanguage === "en" && rule.id === "trash" ? "Trash cards" : rule.label;
const validatePinned = vm.runInNewContext(sourceFunction("validatePinnedQuotas") + "\nvalidatePinnedQuotas", pinnedContext);
const pinnedQuotas = (id, max) => new Map(propertyAPI.countRules.map(rule => [rule.id, {min: 0, max: rule.id === id ? max : 14}]));
assert.throws(() => validatePinned(get("research"), [], pinnedQuotas("trash", 0)), /Forscherin.*Karten entsorgen.*höchstens 0.*bereits 1.*mindestens 1/);
assert.doesNotThrow(() => validatePinned(get("research"), [], pinnedQuotas("trash", 1)));
assert.throws(() => validatePinned(get("research", "chapel"), [], pinnedQuotas("trash", 1)), /Forscherin, Kapelle.*bereits 2/);
assert.throws(() => validatePinned(get("research"), [], pinnedQuotas("cost-4", 0)), /Forscherin.*4 Münzen/);
assert.throws(() => validatePinned(get("research"), [], pinnedQuotas("exp-renaissance", 0)), /Forscherin.*Renaissance/);
assert.doesNotThrow(() => validatePinned(get("moat"), [], pinnedQuotas("defense", 0)));
assert.throws(() => validatePinned(get("moat", "witch"), [], pinnedQuotas("defense", 0)), /Burggraben.*Abwehrkarten/);
assert.doesNotThrow(() => validatePinned(get("beggar"), [], pinnedQuotas("attack-reaction", 0)));
assert.throws(() => validatePinned(get("beggar", "witch"), [], pinnedQuotas("attack-reaction", 0)), /Bettler.*Angriffsreaktionen/);
assert.doesNotThrow(() => validatePinned(get("peasant"), [], pinnedQuotas("attack", 0)));
propertyContext.includeRelated.checked = true;
assert.throws(() => validatePinned(get("peasant"), [], pinnedQuotas("attack", 0)), /Kleinbauer.*Angriff/);
assert.throws(() => validatePinned([], get("foray"), pinnedQuotas("attack", 0)), /Brandschatzung.*Angriff/);
assert.throws(() => validatePinned(get("peasant", "moat"), [], pinnedQuotas("defense", 0)), /Burggraben.*Abwehrkarten/);
assert.throws(() => validatePinned([], get("foray"), pinnedQuotas("defense", 0)), /Brandschatzung.*Abwehrkarten/);
assert.doesNotThrow(() => validatePinned([], get("foray"), pinnedQuotas("cost-3", 0)));
assert.doesNotThrow(() => validatePinned([], get("foray"), pinnedQuotas("exp-plunder", 0)));
assert.doesNotThrow(() => validatePinned(get("moat"), get("way_of_the_mouse"), pinnedQuotas("defense", 0)));
propertyContext.includeRelated.checked = false;
pinnedContext.uiLanguage = "en";
assert.throws(() => validatePinned(get("research"), [], pinnedQuotas("trash", 0)), /Research is pinned.*Trash cards.*at most 0.*at least 1/);

// Reduced, anonymous reproduction of the supplied setup: three fixed sets,
// two random sets, Research pinned, trashing forbidden, and 2–4 Landscapes.
const researchFixture = {
  fixedSets: ["base", "intrigue", "prosperity"],
  randomSets: ["adventures", "menagerie", "nocturne", "promos", "renaissance"],
};
const fixtureFields = values => values.map(value => ({
  value: String(value), min: "0", get valueAsNumber() { return Number(this.value); },
}));
const researchContext = {
  ...propertyContext, includeRelated: {checked: false},
  editionInputs: new Map(), isFirstEdition: card => card.edition === "first",
  randomExpansionCount: {value: "2"}, coloniesMode: {value: "always"}, sheltersMode: {value: "rules"}, requireLoot: {checked: false},
  poolChoices: new Map(all.map(card => [card.id, {value: card.id === "research" ? "pinned" : "random"}])),
  rangeInputs: new Map(), expansionModes: new Map(), uiLanguage: "de",
  pinnedQuotaNotes: new Map(),
  tr: de => de, ruleLabel: rule => rule.label, cardTitle: card => card.text.de[0], expansionName: id => catalog.expansions[id],
};
const fixtureCode = predicateSource + "\n" + ["isValidRangeBound", "rangeNumber", "readRange", "solverCard", "serializeRule",
  "allowsEdition", "expansionInPossiblePool", "pinnedSourcesForRule", "pinnedCountForRule", "effectivePinnedRange",
  "validatePinnedQuotas", "validateRemainingQuotas", "updatePinnedQuotaNotes", "readDrawRequest"].map(sourceFunction).join("\n");
const fixtureAPI = vm.runInNewContext(fixtureCode + "\n({readDrawRequest,countRules,landRules,updatePinnedQuotaNotes})", researchContext);
for (const rule of [...fixtureAPI.countRules, ...fixtureAPI.landRules]) researchContext.pinnedQuotaNotes.set(rule.id,
  {hidden: true, textContent: "", toggleAttribute(name, value) {this[name] = value;}});
for (const rule of fixtureAPI.countRules) researchContext.rangeInputs.set(rule.id, fixtureFields([0, 10]));
for (const id of ["actions", "cards", "buy"]) researchContext.rangeInputs.get(id)[0].value = "1";
researchContext.rangeInputs.get("trash")[1].value = "0";
researchContext.rangeInputs.get("attack")[1].value = "0";
for (const rule of fixtureAPI.landRules) researchContext.rangeInputs.set(rule.id,
  fixtureFields(rule.inherited ? ["", ""] : rule.id === "land-total" ? [2, 4] : [0, 4]));
for (const id of researchContext.expansions) {
  const mode = researchFixture.fixedSets.includes(id) ? "always" : researchFixture.randomSets.includes(id) ? "random" : "off";
  researchContext.expansionModes.set(id, {value: mode});
  researchContext.rangeInputs.set("exp-" + id, fixtureFields(mode === "off" ? [0, 0] : mode === "always" ? [1, 10] : [0, 4]));
}
const beforeConflict = JSON.stringify([...researchContext.rangeInputs].map(([id, fields]) => [id, fields.map(field => field.value)]));
assert.throws(() => fixtureAPI.readDrawRequest(), /Forscherin.*Karten entsorgen/);
assert.equal(JSON.stringify([...researchContext.rangeInputs].map(([id, fields]) => [id, fields.map(field => field.value)])), beforeConflict);
researchContext.rangeInputs.get("trash")[1].value = "1";
const validResearchRequest = fixtureAPI.readDrawRequest();
const fixtureSolver = vm.runInNewContext(worker + "\nanalyzeExpansionLottery");
const researchResult = fixtureSolver(validResearchRequest, () => {});
assert.ok(researchResult.result?.main.includes("research"));
assert.ok(researchResult.result.randomExpansions.includes("renaissance"));

// Priority keeps the original inputs while allowing pinned sources above To.
researchContext.pinnedPriority.checked = true;
researchContext.rangeInputs.get("trash")[1].value = "0";
fixtureAPI.updatePinnedQuotaNotes();
assert.match(researchContext.pinnedQuotaNotes.get("trash").textContent, /1 fest · noch 0–0.*überschreiten/);
assert.equal(researchContext.pinnedQuotaNotes.get("trash")["data-exceeded"], true);
let priorityRequest = fixtureAPI.readDrawRequest();
assert.equal(researchContext.rangeInputs.get("trash")[1].value, "0");
assert.equal(priorityRequest.expansionLottery.required.length, 0);
let priorityResult = fixtureSolver(priorityRequest, () => {}).result;
assert.ok(priorityResult?.main.includes("research"));
assert.equal(priorityResult.main.filter(id => byId.get(id).nativeFilters.includes("Trashing")).length, 1);
assert.equal(priorityResult.randomExpansions.length, 2);
// A pinned card from an off set is the only admission from that set.
researchContext.expansionModes.get("renaissance").value = "off";
priorityRequest = fixtureAPI.readDrawRequest();
assert.ok(priorityRequest.fixed.some(card => card.id === "research"));
assert.ok(priorityRequest.candidates.every(card => card.expansion !== "renaissance"));
priorityResult = fixtureSolver(priorityRequest, () => {}).result;
assert.ok(priorityResult?.main.includes("research"));
assert.equal(priorityResult.main.filter(id => byId.get(id).expansion === "renaissance").length, 1);
// Old-edition pins do not enable the rest of the old edition.
researchContext.poolChoices.get("adventurer").value = "pinned";
priorityRequest = fixtureAPI.readDrawRequest();
assert.ok(priorityRequest.fixed.some(card => card.id === "adventurer"));
assert.ok(!priorityRequest.candidates.some(card => card.id === "chancellor"));
researchContext.poolChoices.get("adventurer").value = "random";
// Six fixed cards leave four slots, with Research as the only allowed trasher.
for (const id of ["village", "smithy", "market", "cellar", "workshop"]) researchContext.poolChoices.get(id).value = "pinned";
priorityResult = fixtureSolver(fixtureAPI.readDrawRequest(), () => {}).result;
assert.equal(priorityResult.main.length, 10);
for (const id of ["research", "village", "smithy", "market", "cellar", "workshop"]) assert.ok(priorityResult.main.includes(id));
assert.equal(priorityResult.main.filter(id => byId.get(id).nativeFilters.includes("Trashing")).length, 1);
// A lower bound that the four remaining slots cannot meet is explained.
researchContext.rangeInputs.get("cards")[0].value = "6";
assert.throws(() => fixtureAPI.readDrawRequest(), /freien Hauptstapelplätzen/);
researchContext.rangeInputs.get("cards")[0].value = "1";
// Pinned Landscapes are admitted despite zero total and source maxima.
researchContext.poolChoices.get("foray").value = "pinned";
researchContext.rangeInputs.set("land-total", fixtureFields([0, 0]));
researchContext.rangeInputs.set("land-type-event", fixtureFields([0, 0]));
priorityRequest = fixtureAPI.readDrawRequest();
assert.equal(priorityRequest.landscapeCount.max, 1);
assert.ok(priorityRequest.landscapePool.some(card => card.id === "foray"));
priorityResult = fixtureSolver(priorityRequest, () => {}).result;
assert.deepEqual(Array.from(priorityResult.landscapes), ["foray"]);
researchContext.poolChoices.get("foray").value = "random";
// Main pile count remains a hard limit.
for (const card of base.slice(0, 12)) researchContext.poolChoices.get(card.id).value = "pinned";
assert.throws(() => fixtureAPI.readDrawRequest(), /höchstens 10/);
for (const choice of researchContext.poolChoices.values()) choice.value = "random";
researchContext.poolChoices.get("research").value = "pinned";
for (const size of [11, 12, 13]) {
  researchContext.kingdomSize = size;
  assert.equal(fixtureSolver(fixtureAPI.readDrawRequest(), () => {}).result.main.length, size);
}
researchContext.kingdomSize = 10;
for (const id of ["foray", "advance", "obelisk", "way_of_the_mouse", "cursed"]) researchContext.poolChoices.get(id).value = "pinned";
assert.throws(() => fixtureAPI.readDrawRequest(), /höchstens 4 Landschaften/);
researchContext.pinnedPriority.checked = false;
fixtureAPI.updatePinnedQuotaNotes();
assert.ok([...researchContext.pinnedQuotaNotes.values()].every(note => note.hidden));

// A random set that is not drawn contributes only its explicit pins. Its
// original minimum is inactive, and its other cards remain out of the pool.
const unselectedPinnedSet = request(get("research"), base.slice(0, 10), {
  pinnedPriority: true,
  expansionLottery: {always: ["base"], required: [], optional: ["renaissance"], count: 0, autoLandscapes: []},
  rules: [rule(get("research"), 3, 4, {conditionalExpansion: "renaissance"})],
});
const noRandomSet = fixtureSolver(unselectedPinnedSet, () => {}).result;
assert.ok(noRandomSet?.main.includes("research"));
assert.equal(noRandomSet.main.filter(id => byId.get(id).expansion === "renaissance").length, 1);

const validationContext = {
  ...propertyContext, allowedKingdomSizes: [10, 11, 12, 13],
  countRules: propertyAPI.countRules, landRules: [], expansionModes: new Map(),
  expansions: Object.keys(catalog.expansions),
  tr: (de) => de, ruleLabel: rule => rule.label,
  validRangePair: propertyAPI.validRangePair,
  isValidRangeBound: (value, max) => Number.isInteger(value) && value >= 0 && value <= max,
};
const parseSettings = vm.runInNewContext(sourceFunction("readSettings") + "\nreadSettings", validationContext);
const legacySettings = {version: 1, ranges: {}, cardChoices: {}, setName: "Test"};
assert.equal(parseSettings(JSON.stringify(legacySettings)).includeRelated, undefined);
assert.equal(parseSettings(JSON.stringify({...legacySettings, pinnedPriority: true})).pinnedPriority, true);
assert.throws(() => parseSettings(JSON.stringify({...legacySettings, pinnedPriority: "true"})), /ungültige/);
assert.equal(parseSettings(JSON.stringify({...legacySettings, includeRelated: true, ranges: {attack: ["0", "14"]}})).includeRelated, true);
assert.throws(() => parseSettings(JSON.stringify({...legacySettings, includeRelated: "true"})), /ungültige/);
assert.throws(() => parseSettings(JSON.stringify({...legacySettings, ranges: {attack: ["0", "14"]}})), /Angriff/);
const settingsToggle = {checked: true};
const applyContext = {
  pinnedPriority: {checked: false},
  includeRelated: settingsToggle, standardKingdomSize: 10, countRules: [], landRules: [],
  poolChoices: new Map(), editionInputs: new Map(), requireLoot: {}, coloniesMode: {}, sheltersMode: {},
  uiLanguage: "de", setName: {},
  setKingdomSize: () => {}, applyRangeSettings: () => {}, applyExpansionModes: () => {},
  refreshPreviews: () => {}, refreshPoolSearch: () => {}, updateEffectSet: () => {},
};
const applyStoredSettings = vm.runInNewContext(sourceFunction("applySettings") + "\napplySettings", applyContext);
applyStoredSettings(legacySettings);
assert.equal(settingsToggle.checked, false, "old settings default the new mode to off");
applyStoredSettings({...legacySettings, includeRelated: true});
assert.equal(settingsToggle.checked, true);
applyStoredSettings({...legacySettings, pinnedPriority: true});
assert.equal(applyContext.pinnedPriority.checked, true);
applyStoredSettings({ranges: {}, cardChoices: {}, setName: "Reset"});
assert.equal(settingsToggle.checked, false, "a reset returns the new mode to off");
assert.equal(applyContext.pinnedPriority.checked, false);

// Exercise the real nested-preview state logic without a browser/renderer.
class TooltipNode {
  constructor(parentElement = null) { this.parentElement = parentElement; this.hidden = true; this.attributes = new Map(); }
  contains(node) { for (let item = node; item; item = item.parentElement) if (item === this) return true; return false; }
  setAttribute(key, value) { this.attributes.set(key, value); }
  removeAttribute(key) { this.attributes.delete(key); }
}
const rootNode = new TooltipNode();
const parentPreview = {button: new TooltipNode(rootNode), panel: new TooltipNode(rootNode)};
const childPreview = {button: new TooltipNode(parentPreview.panel), panel: new TooltipNode(parentPreview.panel)};
const tooltipState = {
  previews: new Set([parentPreview, childPreview]), pinnedTooltips: new Map(), tooltipTriggers: new WeakMap(),
  activePreview: null, clearTimeout, previewCloseTimer: undefined,
  updatePreview: () => {}, positionRelatedPreview: () => {},
};
for (const preview of tooltipState.previews) tooltipState.tooltipTriggers.set(preview.panel, preview.button);
const tooltipAPI = vm.runInNewContext(["pinTooltip", "releaseTooltipPins", "closeCardPreview", "canOpenPreview", "setPreview"]
  .map(sourceFunction).join("\n") + "\n({setPreview,closeCardPreview,canOpenPreview})", tooltipState);
tooltipAPI.setPreview(parentPreview, true);
assert.ok(tooltipAPI.canOpenPreview(childPreview.button));
tooltipAPI.setPreview(childPreview, true);
assert.equal(parentPreview.panel.hidden, false, "opening a nested card leaves the parent open");
assert.ok(tooltipState.pinnedTooltips.has(parentPreview.panel));
assert.ok(tooltipState.pinnedTooltips.has(childPreview.panel));
tooltipAPI.closeCardPreview(childPreview);
assert.equal(parentPreview.panel.hidden, false);
assert.equal(tooltipState.activePreview, parentPreview);
tooltipAPI.setPreview(childPreview, true);
tooltipAPI.closeCardPreview(parentPreview);
assert.equal(parentPreview.panel.hidden, true);
assert.equal(childPreview.panel.hidden, true);
assert.equal(tooltipState.pinnedTooltips.size, 0);

// Test both real export components with a small DOM model. No profile is read
// or written; XML snippets below are synthetic, and clipboard access is mocked.
class ExportNode extends TooltipNode {
  constructor(tag = "div") {
    super(); this.tagName = tag; this.children = []; this.listeners = new Map();
    this.textContent = ""; this.value = ""; this.className = ""; this.dataset = {};
  }
  append(...children) { for (const child of children) { child.parentElement = this; this.children.push(child); } }
  addEventListener(name, fn) { this.listeners.set(name, fn); }
  focus() { this.focused = true; }
  select() { this.selected = true; }
}
const selectable = all.filter(card => card.kingdomPile || isLandscapeCard(card));
assert.equal(selectable.length, 653);
const banContext = {
  cardById: byId, uiLanguage: "de", interfaceLabels: [], banExportViews: [],
  poolChoices: new Map(selectable.map(card => [card.id, {value: "random"}])),
  document: {createElement: tag => new ExportNode(tag)},
  cardTitle: (card, lang) => card.text[lang][0],
};
banContext.tr = (de, en) => banContext.uiLanguage === "en" ? en : de;
let copiedBans;
banContext.navigator = {clipboard: {writeText: async value => {copiedBans = value;}}};
const bansAPI = vm.runInNewContext(["bindInterfaceLabel", "formatBanEntries", "createBanExport", "updateBanExports", "copyText"]
  .map(sourceFunction).join("\n") + "\n({formatBanEntries,createBanExport,updateBanExports})", banContext);
for (const area of ["pool", "result"]) {
  const mount = new ExportNode(); mount.dataset.banExport = area;
  banContext.banExportViews.push(bansAPI.createBanExport(mount));
}
const banViews = banContext.banExportViews;
assert.notEqual(banViews[0].field.id, banViews[1].field.id);
const assertBans = (text, count) => {
  bansAPI.updateBanExports();
  for (const view of banViews) {
    assert.equal(view.field.value, text);
    assert.equal(view.button.disabled, text === "");
    assert.ok(view.count.textContent.startsWith(String(count) + " "));
  }
};
assertBans("", 0);
banContext.poolChoices.get("village").value = "excluded";
banContext.poolChoices.get("cellar").value = "excluded";
const expectedBans = "<bans>512</bans>\n<bans>518</bans>";
assertBans(expectedBans, 2);
assert.equal(bansAPI.formatBanEntries(["cellar", "cellar"]).count, 1);
assert.equal(bansAPI.formatBanEntries(["knights"]).text, "<bans>2332</bans>");
assert.equal(bansAPI.formatBanEntries(["farm"]).text, "<bans>792</bans>");
assert.ok(byId.get("farm").alternateTitles.some(title => title.toLowerCase() === "harem"));
banContext.poolChoices.get("way_of_the_mouse").value = "excluded";
assertBans(expectedBans + "\n<bans>3898</bans>", 3);
// Imported/saved card choices are sufficient; no draw or expansion setting is needed.
for (const field of banContext.poolChoices.values()) field.value = "random";
for (const [id, choice] of Object.entries({adventurer: "excluded", foray: "excluded", moat: "pinned"})) {
  banContext.poolChoices.get(id).value = choice;
}
const restoredBans = bansAPI.formatBanEntries(["adventurer", "foray"]).text;
assertBans(restoredBans, 2);
banContext.uiLanguage = "en";
banContext.interfaceLabels.forEach(render => render());
assertBans(restoredBans, 2);
assert.equal(banViews[0].button.textContent, "Copy ban list");
assert.ok(banViews[0].count.textContent.endsWith(" excluded"));
const invalidIds = new Map(byId);
invalidIds.set("cellar", {...byId.get("cellar"), nativeId: undefined});
assert.throws(() => bansAPI.formatBanEntries(["cellar"], invalidIds), /Cellar/);
invalidIds.set("cellar", {...byId.get("cellar"), nativeId: byId.get("village").nativeId});
assert.throws(() => bansAPI.formatBanEntries(["cellar", "village"], invalidIds), /Ambiguous/);
assert.throws(() => bansAPI.formatBanEntries(["not_a_card"]), /not_a_card/);
const tooMany = new Map(Array.from({length: 701}, (_, i) => ["test_" + i, {nativeId: 256 + i}]));
assert.throws(() => bansAPI.formatBanEntries([...tooMany.keys()], tooMany), /700/);
const originalVillage = byId.get("village");
byId.set("village", {...originalVillage, nativeId: undefined});
banContext.poolChoices.get("village").value = "excluded";
bansAPI.updateBanExports();
assert.ok(banViews.every(view => view.field.value === "" && view.button.disabled && view.message.className === "error"));
byId.set("village", originalVillage);
for (const field of banContext.poolChoices.values()) field.value = "random";
assertBans("", 0);
assert.ok(banViews.every(view => view.message.textContent === ""));
const allBanEntries = bansAPI.formatBanEntries(selectable.map(card => card.id));
assert.equal(allBanEntries.count, 653);
assert.equal(new Set(allBanEntries.text.match(/\d+/g)).size, 653);

// Catch syntax failures in every executable inline script.
for (const match of html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/g)) {
  if (/type="(?:application\/json|text\/plain)"/.test(match[0].slice(0,match[0].indexOf('>')))) continue;
  new vm.Script(match[1]);
}
async function checkBanClipboard() {
  banContext.poolChoices.get("cellar").value = "excluded";
  banContext.poolChoices.get("village").value = "excluded";
  assertBans(expectedBans, 2);
  await banViews[0].button.listeners.get("click")();
  assert.equal(copiedBans, expectedBans);
  assert.equal(banViews[0].message.textContent, "Ban-list entries copied.");
  banContext.navigator.clipboard.writeText = async () => { throw new Error("permission denied"); };
  await banViews[1].button.listeners.get("click")();
  assert.equal(banViews[1].field.focused, true);
  assert.equal(banViews[1].field.selected, true);
  assert.match(banViews[1].message.textContent, /Ctrl\+C/);
}
checkBanClipboard().then(() => console.log("Katalog, Solver, Zusatzkarten, Landschaften, Einstellungen und Bannlisten-IDs/XML-Ausgabe an beiden Stellen samt Kopieren und Fallback geprüft."))
  .catch(error => { console.error(error); process.exitCode = 1; });
