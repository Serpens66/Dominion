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

console.log("Katalog, exakte Zählung, 6+4, Abwehr, Sonderzuordnungen, Prophezeiung, Landschaftsquoten, bedingte Zufallsset-Quoten und Zeitlimit geprüft.");
