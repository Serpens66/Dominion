"use strict";

const setupTargets = {
  young_witch: ["Bannkarte im Vorrat", [2, 3]],
  ferryman: ["Stapel neben dem Vorrat", [3, 4]],
  riverboat: ["Für Flussboot beiseite", [5], true],
  way_of_the_mouse: ["Für den Weg der Maus beiseite", [2, 3], true],
};
const requiresAssignment = new Set([...Object.keys(setupTargets), "druid", "black_market", "obelisk"]);
const has = (card, type) => card.types.includes(type);
const costs = (card, values) => card.cost && !card.cost.potions && !card.cost.debt && values.includes(card.cost.coins);

function shuffled(items) {
  const copy = [...items];
  for (let index = copy.length - 1; index > 0; index--) {
    const other = Math.floor(Math.random() * (index + 1));
    [copy[index], copy[other]] = [copy[other], copy[index]];
  }
  return copy;
}

function matching(cards, rule) {
  if (rule.distinct) return new Set(cards.filter(card => rule.ids.has(card.id)).map(card => card.expansion)).size;
  return cards.filter(card => rule.ids.has(card.id)).length;
}

function fits(cards, rules, attack = false) {
  return rules.every(rule => {
    if (rule.whenAttacked && !attack) return true;
    const count = matching(cards, rule);
    return count >= rule.min && count <= rule.max;
  });
}

function assignSetup(main, landscapes, request, random, tick) {
  const { extras, components, requireLoot, colonies, shelters } = request;
  const byType = type => components.filter(card => has(card, type));
  const order = items => random ? shuffled(items) : items;
  const unused = extras.filter(card => !main.some(pile => pile.id === card.id));
  const tasks = [];
  const material = new Set();
  const componentGroups = new Set();
  let blackMarket;
  let hasAlly = false;
  let hasProphecy = false;
  let loot = [...main, ...landscapes].some(card => card.lootSource);

  function addCardTasks(card) {
    tick();
    if (card.id === "black_market") blackMarket = card;
    const target = setupTargets[card.id];
    if (target) {
      const [role, prices, action] = target;
      tasks.push({ card, role, exclusive: "extra", options: unused.filter(pile =>
        costs(pile, prices) && (!action || (has(pile, "action") && !has(pile, "duration")))) });
    }
    if (card.id === "obelisk" || has(card, "trait")) {
      const trait = has(card, "trait");
      tasks.push({ card, role: "Ziel im Vorrat", exclusive: trait ? "trait" : null,
        options: main.filter(pile => has(pile, "action") || (trait && has(pile, "treasure"))) });
    }
    if (card.id === "druid") {
      const boons = order(byType("boon")).slice(0, 3);
      tasks.push({ card, role: "Drei Gaben beiseite", options: boons.length === 3 ? [boons] : [] });
    }
    if (!hasAlly && has(card, "liaison")) {
      tasks.push({ card: null, role: "Verbündeter", options: byType("ally") });
      hasAlly = true;
    }
    if (!hasProphecy && has(card, "omen")) {
      tasks.push({ card: null, role: "Prophezeiung", options: byType("prophecy") });
      hasProphecy = true;
    }
  }

  for (const card of [...main, ...landscapes]) addCardTasks(card);
  tasks.sort((a, b) => a.options.length - b.options.length);
  const taken = { extra: new Set(), trait: new Set() };
  const assignments = [];

  function assign(index) {
    tick();
    if (index === tasks.length) return !blackMarket || unused.some(card => !taken.extra.has(card.id));
    const task = tasks[index];
    const used = taken[task.exclusive];
    for (const option of order(task.options)) {
      if (used?.has(option.id)) continue;
      used?.add(option.id);
      const targets = Array.isArray(option) ? option : [option];
      assignments.push({ source: task.card?.id, role: task.role, targets: targets.map(card => card.id) });
      const taskCount = tasks.length;
      const priorMarket = blackMarket;
      const priorAlly = hasAlly;
      const priorProphecy = hasProphecy;
      if (task.exclusive === "extra") addCardTasks(option);
      // A prophecy can introduce a second assignment of its own.
      if (option.id === "approaching_army") {
        tasks.push({ card: option, role: "Zusätzlicher Angriffsstapel im Vorrat", exclusive: "extra",
          options: unused.filter(card => has(card, "attack")) });
      }
      if (assign(index + 1)) return true;
      tasks.length = taskCount;
      blackMarket = priorMarket;
      hasAlly = priorAlly;
      hasProphecy = priorProphecy;
      assignments.pop();
      used?.delete(option.id);
    }
    return false;
  }

  if (!assign(0)) return null;
  const assigned = new Set(assignments.flatMap(item => item.targets));
  if (blackMarket) {
    const deck = order(unused.filter(card => !taken.extra.has(card.id))).slice(0, 60);
    assignments.push({ source: blackMarket.id, role: "Schwarzmarkt-Deck", targets: deck.map(card => card.id) });
  }
  const setupCards = [...main, ...landscapes, ...extras.filter(card => assigned.has(card.id)),
    ...components.filter(card => assigned.has(card.id))];
  const landscapeIds = new Set(landscapes.map(card => card.id));
  for (const card of setupCards) {
    tick();
    if (card.lootSource) loot = true;
    if (card.cost?.potions) material.add("Tränke");
    if (card.cost?.debt || card.mechanics.includes("debt")) material.add("Schuldenmarker");
    if (has(card, "liaison")) material.add("Gefallen-Marker");
    if (has(card, "omen")) material.add("Sonnenmarker");
    if (has(card, "reserve")) material.add("Wirtshaustableau");
    if (has(card, "fate") && card.id !== "druid") componentGroups.add("boon");
    if (has(card, "doom")) componentGroups.add("hex");
    if (has(card, "looter")) componentGroups.add("ruins");
    const mechanics = [...card.mechanics, ...(card.pileMechanics ?? [])];
    if (mechanics.includes("horse")) componentGroups.add("horse");
    if (mechanics.includes("coffers")) material.add("Taler");
    if (mechanics.includes("villagers")) material.add("Dorfbewohner");
    if (mechanics.includes("vp") || ["aqueduct", "arena", "basilica", "baths", "battlefield", "colonnade",
        "defiled_shrine", "labyrinth"].includes(card.id)) material.add("Punktemarker");
    if (mechanics.includes("exile")) material.add("Exil Tableau");
    if (mechanics.includes("journey")) material.add("Reise-Marker");
    if (card.mechanics.includes("trade_route")) material.add("Handelsrouten-Tableau und Geldmarker");
    for (const id of card.related) assigned.add(id);
  }
  if (requireLoot && ![...main, ...landscapes].some(card => card.lootSource)) return null;
  if (loot) componentGroups.add("loot");
  const orderForOptions = order(main);
  const withColonies = colonies === "always" || (colonies === "rules" && orderForOptions[0]?.expansion === "prosperity");
  const withShelters = shelters === "always" || (shelters === "rules" && orderForOptions[1]?.expansion === "dark_ages");
  if (withColonies) { assigned.add("platinum"); assigned.add("colony"); }
  if (withShelters) componentGroups.add("shelter");
  for (const type of componentGroups) for (const card of byType(type)) assigned.add(card.id);
  const needsSteamCheck = new Set();
  if (landscapes.length) needsSteamCheck.add("Landschaften");
  for (const assignment of assignments) {
    needsSteamCheck.add(assignment.role + (assignment.source ? " (" + assignment.source + ")" : ""));
  }
  if (withColonies || main.some(card => card.expansion === "prosperity")) needsSteamCheck.add("Platin und Kolonien");
  if (withShelters || main.some(card => card.expansion === "dark_ages")) needsSteamCheck.add("Unterschlüpfe");
  return {
    main: main.map(card => card.id), landscapes: [...landscapeIds], assignments,
    components: [...assigned], material: [...material], withColonies, withShelters,
    needsSteamCheck: [...needsSteamCheck],
  };
}

function chooseLandscapes(main, request, random, tick) {
  const { landscapePool, landscapeRules, landscapeCount } = request;
  const fixed = landscapePool.filter(card => card.choice === "pinned");
  const available = landscapePool.filter(card => card.choice !== "pinned");
  const ordered = random ? shuffled(available) : available;
  const sizes = [];
  for (let size = Math.max(landscapeCount.min, fixed.length); size <= Math.min(landscapeCount.max, landscapePool.length); size++) sizes.push(size);
  const selected = [...fixed];

  function search(index, left) {
    tick();
    if (left > ordered.length - index) return null;
    for (const rule of landscapeRules) {
      const count = matching(selected, rule);
      if (count > rule.max || count + left < rule.min) return null;
    }
    if (!left) return fits(selected, landscapeRules) ? assignSetup(main, selected, request, random, tick) : null;
    for (let next = index; next <= ordered.length - left; next++) {
      selected.push(ordered[next]);
      const result = search(next + 1, left - 1);
      selected.pop();
      if (result) return result;
    }
    return null;
  }

  for (const size of random ? shuffled(sizes) : sizes) {
    const result = search(0, size - fixed.length);
    if (result) return result;
  }
  return null;
}

const timedOut = Symbol("Zeitlimit");

function analyzeKingdom(input, onFound, deadline = Date.now() + 4800, sampleOnly = false) {
  const compile = rules => rules.map(rule => ({ ...rule, ids: new Set(rule.ids) }));
  const request = { ...input, rules: compile(input.rules), landscapeRules: compile(input.landscapeRules) };
  const { fixed, candidates, rules } = request;
  const slots = (request.kingdomSize ?? 10) - fixed.length;
  const mainPool = [...fixed, ...candidates];
  if (slots < 0 || candidates.length < slots ||
      rules.some(rule => !rule.whenAttacked && rule.min > matching(mainPool, rule)) ||
      request.landscapeCount.min > request.landscapePool.length ||
      request.landscapeRules.some(rule => rule.min > matching(request.landscapePool, rule))) {
    return { total: 0n, result: null };
  }
  let steps = 0;
  function tick() {
    if ((++steps & 127) === 0 && Date.now() >= deadline) throw timedOut;
  }
  const coupled = request.requireLoot || (request.landscapeCount.max > 0 && request.landscapePool.length > 0) ||
    mainPool.some(card => requiresAssignment.has(card.id) || has(card, "liaison") || has(card, "omen"));
  let sample = null;
  let drawExact;
  try {
    const order = shuffled(candidates);
    const chosen = [...fixed];
    const remaining = Array.from({ length: order.length + 1 }, () => []);
    for (let index = order.length - 1; index >= 0; index--) {
      remaining[index] = rules.map((rule, n) => (remaining[index + 1][n] ?? 0) + Number(rule.ids.has(order[index].id)));
    }
    function possible(index, left, counts, attack) {
      if (left > order.length - index) return false;
      return rules.every((rule, n) => {
        if (rule.whenAttacked && !attack) return true;
        const matchingLeft = remaining[index][n] ?? 0;
        const room = Math.min(left, matchingLeft);
        const forced = rule.distinct ? 0 : Math.max(0, left - (order.length - index - matchingLeft));
        return counts[n] + forced <= rule.max && counts[n] + room >= rule.min;
      });
    }
    const seedCounts = rules.map(rule => matching(fixed, rule));
    const seedAttack = fixed.some(card => has(card, "attack"));
    const update = (card, counts) => rules.map((rule, n) =>
      counts[n] + Number(rule.ids.has(card.id) && (!rule.distinct ||
        !chosen.some(other => other.expansion === card.expansion && rule.ids.has(other.id)))));
    const memo = new Map();
    function count(index, left, counts, attack) {
      tick();
      if (!possible(index, left, counts, attack)) return 0n;
      if (!left) return 1n;
      // Distinct expansion quotas depend on the already represented expansions.
      const mask = rules.some(rule => rule.distinct)
        ? [...new Set(chosen.map(card => card.expansion))].sort().join("/") : "";
      const key = [index, left, Number(attack), ...counts, mask].join(",");
      if (memo.has(key)) return memo.get(key);
      const card = order[index];
      const next = update(card, counts);
      chosen.push(card);
      const withCard = count(index + 1, left - 1, next, attack || has(card, "attack"));
      chosen.pop();
      const total = withCard + count(index + 1, left, counts, attack);
      if (memo.size >= 250000) throw timedOut;
      memo.set(key, total);
      return total;
    }

    function randomRank(total) {
      const bits = total.toString(2).length;
      let rank;
      do {
        rank = 0n;
        for (let left = bits; left > 0; left -= 26) {
          const size = Math.min(26, left);
          rank = (rank << BigInt(size)) + BigInt(Math.floor(Math.random() * 2 ** size));
        }
      } while (rank >= total);
      return rank;
    }

    function enumerate(index, left, counts, attack, visit) {
      tick();
      if (!possible(index, left, counts, attack)) return false;
      if (!left) return visit(chosen);
      for (let next = index; next <= order.length - left; next++) {
        if (!possible(next, left, counts, attack)) continue;
        const card = order[next];
        const after = update(card, counts);
        chosen.push(card);
        const stop = enumerate(next + 1, left - 1, after, attack || has(card, "attack"), visit);
        chosen.pop();
        if (stop) return true;
      }
      return false;
    }

    const visits = visit => enumerate(0, slots, seedCounts, seedAttack, visit);
    visits(main => {
      sample = chooseLandscapes(main, request, true, tick);
      if (!sample) return false;
      onFound(sample);
      return true;
    });
    if (!sample) return { total: 0n, result: null };
    if (sampleOnly) return { total: null, result: sample };

    if (coupled) {
      let total = 0n;
      let selected;
      visits(main => {
        const setup = chooseLandscapes(main, request, false, tick);
        if (setup) {
          total++;
          if (randomRank(total) === 0n) selected = [...main];
        }
        return false;
      });
      return { total, result: chooseLandscapes(selected, request, true, tick) };
    }

    const total = count(0, slots, seedCounts, seedAttack);
    drawExact = (drawDeadline = Date.now() + 4800) => {
      deadline = drawDeadline;
      let rank = randomRank(total);
      let left = slots;
      let counts = seedCounts;
      let attack = seedAttack;
      chosen.length = fixed.length;
      for (let index = 0; left; index++) {
        const card = order[index];
        const next = update(card, counts);
        const nextAttack = attack || has(card, "attack");
        chosen.push(card);
        const completions = count(index + 1, left - 1, next, nextAttack);
        if (rank < completions) {
          left--;
          counts = next;
          attack = nextAttack;
        } else {
          chosen.pop();
          rank -= completions;
        }
      }
      const result = chooseLandscapes(chosen, request, true, tick);
      chosen.length = fixed.length;
      return result;
    };
    return { total, result: drawExact(deadline), drawExact };
  } catch (error) {
    if (error !== timedOut) throw error;
    return { total: null, result: sample };
  }
}

function analyzeExpansionLottery(request, onFound, deadline = Date.now() + 4800) {
  const { always, required, optional, count, autoLandscapes } = request.expansionLottery;
  const remaining = count - required.length;
  if (remaining < 0 || remaining > optional.length) return { total: 0n, result: null };
  const combinations = [];
  const extra = [];

  function collect(index, remaining) {
    if (!remaining) {
      combinations.push([...extra]);
      return;
    }
    for (let next = index; next <= optional.length - remaining; next++) {
      extra.push(optional[next]);
      collect(next + 1, remaining - 1);
      extra.pop();
    }
  }

  collect(0, remaining);
  const inherited = new Set(autoLandscapes);
  for (const picked of shuffled(combinations)) {
    if (Date.now() >= deadline) return { total: null, result: null };
    const randomExpansions = [...required, ...picked];
    const selected = new Set([...always, ...randomExpansions]);
    const inPool = card => selected.has(card.expansion);
    const trial = {
      ...request,
      rules: request.rules.filter(rule => !rule.conditionalExpansion || selected.has(rule.conditionalExpansion)),
      candidates: request.candidates.filter(inPool),
      extras: request.extras.filter(inPool),
      landscapePool: request.landscapePool.filter(card => !inherited.has(card.expansion) || inPool(card)),
    };
    const addExpansions = result => ({ ...result, randomExpansions });
    const outcome = analyzeKingdom(trial, result => onFound(addExpansions(result)), deadline, true);
    if (outcome.result) return { total: null, result: addExpansions(outcome.result) };
    if (outcome.total === null) return outcome;
  }
  return { total: 0n, result: null };
}

let nextExactDraw;
if (typeof self !== "undefined") self.onmessage = ({ data }) => {
  try {
    if (data.type === "draw" && nextExactDraw) {
      self.postMessage({ type: "draw", result: nextExactDraw() });
      return;
    }
    const analyze = data.request.expansionLottery ? analyzeExpansionLottery : analyzeKingdom;
    const result = analyze(data.request, sample => self.postMessage({ type: "found", result: sample }));
    nextExactDraw = result.drawExact;
    self.postMessage({ type: "done", total: result.total?.toString() ?? null,
      result: result.result, reusable: Boolean(nextExactDraw) });
  } catch (error) {
    self.postMessage({ type: "error", message: error === timedOut ? "Zeitlimit erreicht." : error.message });
  }
};

