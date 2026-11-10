const fs=require("node:fs"), assert=require("node:assert/strict"),vm=require("node:vm");
const path=require("node:path");
const html=fs.readFileSync(path.join(__dirname,"Dominion-Generator.html"),"utf8");
const worker=fs.readFileSync(path.join(__dirname,"generator_solver.js"),"utf8");
const api=vm.runInNewContext(worker+"\n({analyzeKingdom,assignSetup,chooseLandscapes})");
const catalog=JSON.parse(html.match(/id="catalog">([\s\S]+?)<\/script>/)[1]);
const all=catalog.cards, byId=new Map(all.map(c=>[c.id,c])), get=(...ids)=>ids.map(id=>{assert.ok(byId.has(id),id);return byId.get(id);});
const base=all.filter(c=>c.kingdomPile&&c.expansion==="base"&&c.edition!=="first");
const intrigue=all.filter(c=>c.kingdomPile&&c.expansion==="intrigue"&&c.edition!=="first");
const allPiles=all.filter(c=>c.kingdomPile);
const components=all.filter(c=>!c.kingdomPile);
assert.equal(all.length,853);
assert.equal(allPiles.length,498);
assert.equal(base.length,26);
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
console.log("Katalog, exakte Zählung, 6+4, Abwehr, Sonderzuordnungen, Prophezeiung, Landschaftsquoten und Zeitlimit geprüft.");
