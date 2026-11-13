"use strict";
// Real-browser layout checks without npm dependencies. Browsers use isolated
// temporary profiles; the generator and the user's browser profile stay intact.
const fs = require("node:fs"), path = require("node:path"), os = require("node:os");
const http = require("node:http"), cp = require("node:child_process"), assert = require("node:assert/strict");
const root = __dirname;
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
const requested = process.argv.slice(2).find(arg => !arg.startsWith("--")) ?? "both";
assert.ok(["both", "edge", "firefox"].includes(requested), "Usage: node test_card_layout.js [both|edge|firefox] [--smoke]");
const smoke = process.argv.includes("--smoke");
const browsers = {
  edge: "C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe",
  firefox: "C:/Program Files/Mozilla Firefox/firefox.exe",
};

async function connect(url) {
  const ws = new WebSocket(url), pending = new Map();
  let sequence = 0;
  await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = reject; });
  ws.onmessage = event => {
    const data = JSON.parse(event.data), task = pending.get(data.id);
    if (!task) return;
    pending.delete(data.id);
    clearTimeout(task.timer);
    if (data.error || data.type === "error") task.reject(new Error(JSON.stringify(data)));
    else task.resolve(data.result);
  };
  ws.onclose = () => { for (const task of pending.values()) { clearTimeout(task.timer); task.reject(new Error("Browser connection closed")); } };
  return {
    send(method, params = {}, timeout = 120000) {
      const id = ++sequence;
      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => { pending.delete(id); reject(new Error("Browser command timed out: " + method)); }, timeout);
        pending.set(id, {resolve, reject, timer}); ws.send(JSON.stringify({id, method, params}));
      });
    }, close: () => ws.close(),
  };
}

async function freePort() {
  const s = http.createServer();
  await new Promise(resolve => s.listen(0, "127.0.0.1", resolve));
  const port = s.address().port;
  await new Promise(resolve => s.close(resolve));
  return port;
}

async function launch(name, url) {
  assert.ok(fs.existsSync(browsers[name]), "Missing browser: " + browsers[name]);
  const port = await freePort(), profile = fs.mkdtempSync(path.join(os.tmpdir(), "dominion-layout-" + name + "-"));
  const args = name === "edge"
    ? ["--headless=new", "--no-first-run", "--user-data-dir=" + profile, "--remote-debugging-port=" + port, "about:blank"]
    : ["--headless", "--no-remote", "--profile", profile, "--remote-debugging-port", String(port)];
  const child = cp.spawn(browsers[name], args, {windowsHide: true, stdio: "ignore"});
  async function cleanProfile() {
    // Resolve and verify this exact mkdtemp directory before recursive cleanup.
    const tempRoot = fs.realpathSync.native(os.tmpdir());
    const target = fs.realpathSync.native(profile);
    if (!target.toLowerCase().startsWith((tempRoot + path.sep).toLowerCase()) || !path.basename(target).startsWith("dominion-layout-" + name + "-")) {
      throw new Error("Refusing to remove an unexpected browser profile: " + target);
    }
    await delay(250);
    try { fs.rmSync(target, {recursive: true, force: true, maxRetries: 5, retryDelay: 200}); }
    catch { console.warn("Temporary browser profile remains locked: " + target); }
  }
  let protocol, lastError;
  for (let attempt = 0; attempt < 80; attempt++) {
    try {
      const wsUrl = name === "edge"
        ? (await (await fetch(`http://127.0.0.1:${port}/json/version`)).json()).webSocketDebuggerUrl
        : `ws://127.0.0.1:${port}/session`;
      protocol = await connect(wsUrl); break;
    } catch (error) { lastError = error; await delay(200); }
  }
  if (!protocol) { child.kill(); throw lastError; }
  if (name === "edge") {
    const browser = protocol;
    const {targetId} = await browser.send("Target.createTarget", {url: "about:blank"});
    const pages = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
    const page = await connect(pages.find(p => p.id === targetId).webSocketDebuggerUrl);
    await page.send("Page.enable");
    await page.send("Page.navigate", {url});
    await browser.send("Target.activateTarget", {targetId});
    return {
      version: (await browser.send("Browser.getVersion")).product,
      async evaluate(expression) {
        const result = await page.send("Runtime.evaluate", {expression, awaitPromise: true, returnByValue: true});
        if (result.exceptionDetails) throw new Error(JSON.stringify(result.exceptionDetails));
        return result.result.value;
      },
      async viewport(width) { await page.send("Emulation.setDeviceMetricsOverride", {width, height: 1000, deviceScaleFactor: 1, mobile: false}); },
      async screenshot() { return (await page.send("Page.captureScreenshot", {format: "png"})).data; },
      async close() { try { await browser.send("Browser.close", {}, 5000); } catch {} page.close(); browser.close(); child.kill(); await cleanProfile(); },
    };
  }
  const {capabilities} = await protocol.send("session.new", {capabilities: {}});
  const {context} = await protocol.send("browsingContext.create", {type: "tab"});
  await protocol.send("browsingContext.navigate", {context, url, wait: "complete"});
  await protocol.send("browsingContext.activate", {context});
  return {
    version: capabilities.browserName + " " + capabilities.browserVersion,
    async evaluate(expression) {
      const result = await protocol.send("script.evaluate", {expression, target: {context}, awaitPromise: true, resultOwnership: "none"});
      if (result.type === "exception") throw new Error(JSON.stringify(result.exceptionDetails));
      return result.result.value;
    },
    async viewport(width) { await protocol.send("browsingContext.setViewport", {context, viewport: {width, height: 1000}, devicePixelRatio: 1}); },
    async screenshot() { return (await protocol.send("browsingContext.captureScreenshot", {context, origin: "viewport"})).data; },
    async close() { try { await protocol.send("browser.close", {}, 5000); } catch {} protocol.close(); child.kill(); await cleanProfile(); },
  };
}

async function exercisePool(browser) {
  const result = JSON.parse(await browser.evaluate(`(() => {
    const failures = []; let checks = 0;
    const check = (ok, kind) => { checks++; if (!ok) failures.push({kind:'pool-' + kind}); };
    const saved = collectSettings(), query = poolSearch.value, filters = [...selectedPoolProperties];
    const open = poolGroups.map(group => [group.details, group.details.open]);
    const search = (text = '', properties = []) => {
      poolSearch.value = text; selectedPoolProperties.clear();
      properties.forEach(id => selectedPoolProperties.add(id)); refreshPoolSearch();
    };
    try {
      search();
      check(poolGroups.length === expansions.length && new Set(poolGroups.map(group => group.expansion)).size === expansions.length, 'unique-expansions');
      const promo = poolGroups.find(group => group.expansion === 'promos');
      const main = promo.sections.find(section => section.area === 'pool');
      const land = promo.sections.find(section => section.area === 'land-pool');
      check(main.items.length === 11 && land.items.length === 1 && land.items[0].card.id === 'summon', 'twelve-promos');
      check(promo.items.length === 12 && promo.details.querySelectorAll('details').length === 0, 'flat-sections');
      check(promo.sections[0] === main && promo.sections[1] === land, 'section-order');
      check(new Set([...poolChoices.values()].map(field => field.id)).size === poolChoices.size, 'unique-choice-controls');
      promo.details.open = false;
      search('summon');
      check(main.element.hidden && !land.element.hidden && !promo.details.hidden && promo.details.open, 'landscape-search');
      check(poolSearchHits === 1 && land.items[0].item.hidden === false, 'search-count');
      search();
      check(!main.element.hidden && !land.element.hidden && !promo.details.open, 'search-reset');
      search('', ['land-type-event']);
      check(main.element.hidden && !land.element.hidden && !land.items[0].item.hidden, 'landscape-property-filter');
      check(poolGroups.every(group => group.items.every(entry => entry.item.hidden || entry.area === 'land-pool')), 'property-role');
      search('black_market');
      check(!main.element.hidden && land.element.hidden, 'main-search');
      search();
      for (const id of promo.items.map(entry => entry.card.id)) poolChoices.get(id).value = 'random';
      poolChoices.get('black_market').value = 'pinned';
      poolChoices.get('summon').value = 'excluded';
      const settings = collectSettings();
      check(settings.cardChoices.black_market === 'pinned' && settings.cardChoices.summon === 'excluded', 'save-both-sections');
      poolChoices.get('black_market').value = 'random'; poolChoices.get('summon').value = 'random';
      applySettings(settings);
      check(poolChoices.get('black_market').value === 'pinned' && poolChoices.get('summon').value === 'excluded', 'restore-both-sections');
      poolChoices.get('black_market').value = 'excluded'; poolChoices.get('summon').value = 'pinned';
      check(collectSettings().cardChoices.summon === 'pinned' && collectSettings().cardChoices.black_market === 'excluded', 'independent-choices');
      for (const entry of promo.items) entry.choice.value = 'random';
      pinnedPriority.checked = false;
      rangeInputs.get('exp-promos')[0].value = '0'; rangeInputs.get('exp-promos')[1].value = '0';
      expansionModes.get('promos').value = 'off';
      rangeInputs.get('land-exp-promos')[0].value = '0'; rangeInputs.get('land-exp-promos')[1].value = '1';
      setInterfaceLanguage('de'); updatePoolGroups();
      check(promo.summaryText.textContent.includes('1 verfügbar'), 'explicit-landscape-quota');
      rangeInputs.get('land-exp-promos')[1].value = '0'; updatePoolGroups();
      check(promo.summaryText.textContent.includes('0 verfügbar'), 'disabled-expansion-and-landscapes');
      rangeInputs.get('exp-promos')[1].value = '10'; expansionModes.get('promos').value = 'always';
      rangeInputs.get('land-exp-promos')[0].value = ''; rangeInputs.get('land-exp-promos')[1].value = '';
      updatePoolGroups();
      check(promo.summaryText.textContent.includes('12 verfügbar'), 'combined-availability');
      poolChoices.get('black_market').value = 'pinned'; poolChoices.get('summon').value = 'excluded'; updatePoolGroups();
      check(promo.summaryText.textContent.includes('11 verfügbar') && promo.summaryText.textContent.includes('1 fest') && promo.summaryText.textContent.includes('1 ausgeschlossen'), 'combined-counts');
      for (const language of ['en', 'de']) {
        setInterfaceLanguage(language);
        for (const group of poolGroups) for (const section of group.sections) {
          const titles = [...section.list.children].map(item => {
            const entry = section.items.find(entry => entry.item === item);
            return cardTitle(entry.card, language);
          });
          check(titles.every((title, i) => !i || titles[i-1].localeCompare(title, uiLocale()) <= 0), 'sort-' + group.expansion + '-' + section.area + '-' + language);
        }
        check(main.heading.textContent === (language === 'de' ? 'Hauptstapel' : 'Kingdom piles') && land.heading.textContent === (language === 'de' ? 'Landschaften' : 'Landscapes'), 'translated-headings-' + language);
      }
    } finally {
      applySettings(saved); search(query, filters); updatePoolGroups();
      for (const [details, wasOpen] of open) details.open = wasOpen;
    }
    return JSON.stringify({checks, failures});
  })()`));
  await browser.evaluate(`(() => {
    const promo = poolGroups.find(group => group.expansion === 'promos');
    const states = []; let element = promo.details;
    while (element) { if (element.tagName === 'DETAILS') { states.push([element, element.open]); element.open = true; } element = element.parentElement; }
    window.__poolLayoutStates = states;
  })()`);
  result.screenshots = [];
  try {
    for (const width of [390, 1920]) {
      await browser.viewport(width); await delay(80);
      const geometry = JSON.parse(await browser.evaluate(`(() => {
        const promo = poolGroups.find(group => group.expansion === 'promos');
        promo.details.scrollIntoView();
        const [main, land] = promo.sections;
        const gap = land.heading.getBoundingClientRect().top - main.element.getBoundingClientRect().bottom;
        const style = getComputedStyle(land.heading), halfLine = (parseFloat(style.lineHeight) || parseFloat(style.fontSize) * 1.2) / 2;
        return JSON.stringify({gap, halfLine, overflow:document.documentElement.scrollWidth > innerWidth + 1,
          mainHeight:main.element.getBoundingClientRect().height,landHeight:land.element.getBoundingClientRect().height,
          mainCardWidth:main.items[0].item.getBoundingClientRect().width,landCardWidth:land.items[0].item.getBoundingClientRect().width});
      })()`));
      result.checks++;
      if (geometry.overflow || geometry.gap < -1 || geometry.gap > geometry.halfLine + 1 || !geometry.mainHeight || !geometry.landHeight || Math.abs(geometry.mainCardWidth - geometry.landCardWidth) > 1) {
        result.failures.push({kind:'pool-section-layout', width, ...geometry});
      }
      const screenshot = path.join(os.tmpdir(), 'dominion-pool-' + browser.version.split(/[ /]/)[0] + '-' + width + '.png');
      fs.writeFileSync(screenshot, Buffer.from(await browser.screenshot(), 'base64'));
      result.screenshots.push(screenshot);
    }
  } finally {
    await browser.evaluate("for(const [element,open] of window.__poolLayoutStates)element.open=open;delete window.__poolLayoutStates;");
  }
  return result;
}

async function exerciseLotteryWeights(browser) {
  const result = JSON.parse(await browser.evaluate(`(() => {
    const failures = []; let checks = 0;
    const check = (ok, kind) => { checks++; if (!ok) failures.push({kind:'lottery-weight-' + kind}); };
    const saved = collectSettings(), language = uiLanguage;
    const ids = ['intrigue', 'prosperity', 'seaside'];
    const modes = Object.fromEntries(expansions.map(id => [id, id === 'base' ? 'always' : ids.includes(id) ? 'random' : 'off']));
    const cancel = () => { clearTimeout(ruleCheckTimer); ruleCheckTimer = undefined; stopAnalysis(); };
    const input = (id, value) => {
      const field = expansionWeightInputs.get(id).field;
      field.value = String(value); field.dispatchEvent(new Event('input', {bubbles:true}));
      field.dispatchEvent(new Event('change', {bubbles:true})); cancel();
    };
    const percent = id => {
      const match = expansionWeightInputs.get(id).chance.textContent.match(/[0-9]+(?:[,.][0-9]+)?/);
      return match ? Number(match[0].replace(',', '.')) : NaN;
    };
    try {
      cancel();
      applyExpansionModes(modes, 1);
      check(expansions.every(id => expansionWeights.get(id) === 1), 'initial-default-one');
      check(ids.every(id => percent(id) === 33.3), 'rounded-equal-chances');
      const first = expansionWeightInputs.get(ids[0]);
      check(!first.wrapper.hidden && !first.field.disabled && first.field.min === '0' && first.field.max === '1000' && first.field.step === '0.1', 'relative-input');
      const base = expansionWeightInputs.get('base');
      check(base.wrapper.hidden && base.field.getBoundingClientRect().height === 0 && base.chance.getBoundingClientRect().height === 0, 'always-weight-and-chance-hidden');
      check(first.field.getAttribute('aria-label')?.length > 0 || first.field.labels?.length > 0, 'accessible-label');
      preparedDraw = {result:{fixture:true}};
      input(ids[0], 5); input(ids[1], 3); input(ids[2], 2);
      check(preparedDraw === null, 'input-invalidates-draw');
      check(ids.every((id,i) => expansionWeights.get(id) === [5,3,2][i] && Number(expansionWeightInputs.get(id).field.value) === [5,3,2][i]), 'independent-five-three-two-inputs');
      check(ids.every((id,i) => percent(id) === [50,30,20][i]), 'five-three-two-chances');
      input(ids[0], 5.1);
      check(expansionWeights.get(ids[0]) === 5.1 && expansionWeights.get(ids[1]) === 3 && expansionWeights.get(ids[2]) === 2, 'tenths-without-other-edits');
      input(ids[0], 1000);
      check(expansionWeights.get(ids[0]) === 1000 && expansionWeights.get(ids[1]) === 3 && expansionWeights.get(ids[2]) === 2, 'maximum-weight');
      input(ids[0], 0);
      check(expansionWeights.get(ids[0]) === 0 && percent(ids[0]) === 0 && percent(ids[1]) === 60 && percent(ids[2]) === 40, 'zero-weight');
      setExpansionMode('adventures', 'random'); input('adventures', 17.5); setExpansionMode('adventures', 'off');
      const weighted = collectSettings(), serialized = JSON.stringify(weighted);
      check(weighted.expansionWeights === undefined && weighted.expansionRelativeWeights.adventures === 17.5 && Object.keys(weighted.expansionRelativeWeights).length === expansions.length, 'save-active-and-inactive');
      check(JSON.stringify(readSettings(serialized).expansionRelativeWeights) === JSON.stringify(weighted.expansionRelativeWeights), 'validate-export');
      equalExpansionWeights.click(); cancel();
      check(ids.every(id => expansionWeights.get(id) === 1 && percent(id) === 33.3) && expansionWeights.get('adventures') === 17.5, 'equal-active-only');
      applySettings(readSettings(serialized));
      check(expansions.every(id => expansionWeights.get(id) === weighted.expansionRelativeWeights[id]), 'settings-roundtrip');
      setExpansionMode('adventures', 'random');
      check(expansionWeights.get('adventures') === 17.5 && Number(expansionWeightInputs.get('adventures').field.value) === 17.5, 'mode-restores-weight');
      setExpansionMode('adventures', 'off');
      const legacy = {...weighted, expansionWeights:{intrigue:500,prosperity:300,seaside:200}};
      delete legacy.expansionRelativeWeights;
      applySettings(readSettings(JSON.stringify(legacy)));
      check(ids.every((id,i) => expansionWeights.get(id) === [50,30,20][i] && percent(id) === [50,30,20][i]) && expansionWeights.get('adventures') === 1, 'legacy-percentage-migration');
      delete legacy.expansionWeights;
      applySettings(readSettings(JSON.stringify(legacy)));
      check(expansions.every(id => expansionWeights.get(id) === 1) && ids.every(id => percent(id) === 33.3), 'legacy-no-weights-default');
      const rejects = weights => {
        try { readSettings(JSON.stringify({...weighted, expansionRelativeWeights:weights})); return false; } catch { return true; }
      };
      check(rejects({...weighted.expansionRelativeWeights, [ids[0]]:-1}), 'reject-negative');
      check(rejects({...weighted.expansionRelativeWeights, [ids[0]]:1000.1}), 'reject-over-maximum');
      check(rejects({...weighted.expansionRelativeWeights, [ids[0]]:1.25}), 'reject-extra-precision');
      check(rejects({...weighted.expansionRelativeWeights, [ids[0]]:'5'}), 'reject-nonnumeric');
      check(rejects({...weighted.expansionRelativeWeights, unknown_expansion:1}), 'reject-unknown-expansion');
      const partial = {...weighted, expansionRelativeWeights:{intrigue:5}};
      applySettings(readSettings(JSON.stringify(partial)));
      check(expansionWeights.get('intrigue') === 5 && expansions.filter(id => id !== 'intrigue').every(id => expansionWeights.get(id) === 1), 'partial-table-defaults');
      applyExpansionModes(modes, 1); input(ids[0], 5); input(ids[1], 3); input(ids[2], 2);
      const before = new Map(expansionWeights);
      setExpansionMode('adventures', 'random');
      check(expansionWeights.get('adventures') === 1 && ids.every(id => expansionWeights.get(id) === before.get(id)), 'add-no-renormalization');
      setExpansionMode('adventures', 'off');
      check(expansionWeightInputs.get('adventures').wrapper.hidden && ids.every(id => expansionWeights.get(id) === before.get(id)), 'remove-no-renormalization');
      setExpansionMode(ids[1], 'off'); setExpansionMode(ids[2], 'off');
      check(expansionWeights.get(ids[0]) === 5 && !first.field.disabled && percent(ids[0]) === 100, 'single-editable-with-full-chance');
      input(ids[0], 7);
      check(expansionWeights.get(ids[0]) === 7 && percent(ids[0]) === 100, 'single-keeps-raw-weight');
      setExpansionMode(ids[1], 'random');
      check(expansionWeights.get(ids[0]) === 7 && expansionWeights.get(ids[1]) === 3 && percent(ids[0]) === 70 && percent(ids[1]) === 30, 'readd-keeps-weight');
      enableRandomExpansions.click(); cancel();
      const randomIds = expansions.filter(id => expansionModes.get(id).value === 'random');
      check(randomIds.length === expansions.length - 1 && ids.every(id => expansionWeights.get(id) === (id === ids[0] ? 7 : before.get(id))) && expansions.filter(id => !ids.includes(id)).every(id => expansionWeights.get(id) === 1), 'bulk-keeps-inputs');
      applyExpansionModes(modes, 3); input(ids[0], 1); input(ids[1], 0); input(ids[2], 0);
      let insufficient = '';
      try { readDrawRequest(); } catch (error) { insufficient = error.message; }
      check(/positiv|positive|Gewicht|weight|über 0|above 0/.test(insufficient), 'too-few-positive-error');
      applyExpansionModes(modes, 1); ids.forEach(id => input(id, 0));
      let allZero = '';
      try { readDrawRequest(); } catch (error) { allZero = error.message; }
      check(/positiv|positive|Gewicht|weight|über 0|above 0/.test(allZero), 'all-zero-error');
      for (const lang of ['en', 'de']) {
        setInterfaceLanguage(lang);
        const label = first.field.getAttribute('aria-label') ?? first.field.labels?.[0]?.textContent ?? '';
        check(label.includes(lang === 'en' ? 'weight' : 'gewicht') && equalExpansionWeights.textContent.includes(lang === 'en' ? 'Equal' : 'gleich'), 'translated-controls-' + lang);
      }
    } finally {
      cancel(); applySettings(saved); setInterfaceLanguage(language); saveSettings(); updatePoolGroups();
    }
    return JSON.stringify({checks, failures});
  })()`));
  await browser.evaluate(`(() => {
    window.__weightLayoutSettings = collectSettings();
    window.__weightLayoutFontSize = document.documentElement.style.fontSize;
    const panel = document.getElementById('expansion-settings'); window.__weightLayoutOpen = panel.open; panel.open = true;
    const ids = ['intrigue','prosperity','seaside'];
    applyExpansionModes(Object.fromEntries(expansions.map(id => [id,id === 'base' ? 'always' : ids.includes(id) ? 'random' : 'off'])), 1, {intrigue:5,prosperity:3,seaside:2});
    window.__weightLayoutLanguage = uiLanguage; setInterfaceLanguage('en');
  })()`);
  result.screenshots = [];
  try {
    const scenarios = [390, 900, 1920].flatMap(width => ['de','en'].map(language => ({width,language,zoom:100})));
    scenarios.push({width:1920,language:'de',zoom:125});
    for (const {width,language,zoom} of scenarios) {
      await browser.viewport(width);
      await browser.evaluate(`document.documentElement.style.fontSize='${zoom}%';setInterfaceLanguage('${language}');`);
      await delay(80);
      const geometry = JSON.parse(await browser.evaluate(`(() => {
        const field = expansionWeightInputs.get('intrigue').field; field.scrollIntoView({block:'center'});
        const overlaps = (a,b) => a.left < b.right - 1 && a.right > b.left + 1 && a.top < b.bottom - 1 && a.bottom > b.top + 1;
        const center = rect => rect.top + rect.height / 2;
        const fields = ['intrigue','prosperity','seaside'].map((id,index) => {
          const value = [1,25,23.9][index]; editExpansionWeight(id,value);
          const entry = expansionWeightInputs.get(id), rect = entry.field.getBoundingClientRect();
          const chance = entry.chance.getBoundingClientRect(), selection = entry.field.closest('.expansion-selection');
          const cell = selection.getBoundingClientRect(), mode = expansionModes.get(id).getBoundingClientRect();
          const name = selection.previousElementSibling, from = selection.nextElementSibling, to = from.nextElementSibling;
          const nameBox = name.getBoundingClientRect(), fromBox = from.getBoundingClientRect(), toBox = to.getBoundingClientRect();
          const edition = name.querySelector('.edition-option')?.getBoundingClientRect();
          const table = selection.closest('.quota-table');
          const title = name.querySelector('.range-rule-title') ?? name.querySelector('button');
          const titleStyle = getComputedStyle(title), titleBox = title.getBoundingClientRect();
          const titleLine = parseFloat(titleStyle.lineHeight) || parseFloat(titleStyle.fontSize) * 1.5;
          const style = getComputedStyle(entry.field), canvas = document.createElement('canvas');
          const context = canvas.getContext('2d'); context.font = style.font;
          const compactWidth = context.measureText('00').width + parseFloat(style.paddingLeft) + parseFloat(style.paddingRight) + 2;
          setExpansionMode(id,'always');
          const always = selection.getBoundingClientRect(), afterFrom = from.getBoundingClientRect(), afterTo = to.getBoundingClientRect();
          const alwaysMode = expansionModes.get(id).getBoundingClientRect();
          setExpansionMode(id,'random');
          return {visible:!entry.wrapper.hidden && rect.height > 0 && chance.height > 0,width:rect.width,compactWidth,
            inline:Math.abs(center(mode)-center(rect)) <= 2 && Math.abs(center(rect)-center(chance)) <= 2,
            ordered:chance.right <= rect.left + 1 && rect.right <= mode.left + 1 && mode.right <= fromBox.left + 1,
            within:chance.left >= cell.left - 1 && mode.right <= cell.right + 1,
            noOverlap:![nameBox,edition,fromBox,toBox].filter(Boolean).some(box => overlaps(box,cell)),
            sameHeight:Math.abs(always.height-cell.height) <= 1,
            alignedMode:Math.abs(alwaysMode.right-mode.right) <= 1 && Math.abs(mode.right-cell.right) <= 1,
            stableBounds:Math.abs(afterFrom.left-fromBox.left) <= 1 && Math.abs(afterTo.left-toBox.left) <= 1,
            readableName:titleBox.height <= titleLine * 2 + 1,
            columns:getComputedStyle(table).gridTemplateColumns.split(' ').length,
            oneInput:selection.querySelectorAll('input').length === 1,
            tooltip:entry.field.title.replace(',', '.').includes(String(value)),
            value:entry.field.value};
        });
        return JSON.stringify({viewport:innerWidth,scroll:document.documentElement.scrollWidth,fields});
      })()`));
      result.checks++;
      if (geometry.scroll > geometry.viewport + 1 || geometry.fields.some(field => !field.visible || !field.inline || !field.ordered || !field.within || !field.noOverlap || !field.sameHeight || !field.alignedMode || !field.stableBounds || !field.oneInput || !field.tooltip || !field.readableName || ![3,4].includes(field.columns) || field.width < 18 || field.width > field.compactWidth + 12)) {
        result.failures.push({kind:'lottery-weight-inline-layout',width,language,zoom,...geometry});
      }
      const screenshot = path.join(os.tmpdir(), 'dominion-weights-' + browser.version.split(/[ /]/)[0] + '-' + width + '-' + language + '-' + zoom + '.png');
      fs.writeFileSync(screenshot, Buffer.from(await browser.screenshot(), 'base64')); result.screenshots.push(screenshot);
    }
  } finally {
    await browser.evaluate("document.documentElement.style.fontSize=window.__weightLayoutFontSize;applySettings(window.__weightLayoutSettings);setInterfaceLanguage(window.__weightLayoutLanguage);document.getElementById('expansion-settings').open=window.__weightLayoutOpen;updatePoolGroups();delete window.__weightLayoutSettings;delete window.__weightLayoutLanguage;delete window.__weightLayoutOpen;delete window.__weightLayoutFontSize;");
  }
  return result;
}

async function exercise(browser) {
  for (let i = 0; i < 100; i++) {
    if (await browser.evaluate("typeof fitCardFace === 'function' && typeof catalog !== 'undefined'")) break;
    await delay(100);
  }
  const report = {browser: browser.version, checks: 0, failures: [], pageWidths: []};
  if (process.env.DOMINION_LAYOUT_PROGRESS) console.error(browser.version, "pool checks");
  report.pool = await exercisePool(browser);
  report.checks += report.pool.checks;
  report.failures.push(...report.pool.failures);
  if (process.env.DOMINION_LAYOUT_PROGRESS) console.error(browser.version, "weight checks");
  report.weights = await exerciseLotteryWeights(browser);
  report.checks += report.weights.checks;
  report.failures.push(...report.weights.failures);
  await browser.evaluate("themeSelect.value='dark';themeSelect.dispatchEvent(new Event('change'))");
  for (const width of [390, 900, 1920, 2560]) {
    await browser.viewport(width);
    await delay(80);
    const sizes = JSON.parse(await browser.evaluate("JSON.stringify({viewport: innerWidth, scroll: document.documentElement.scrollWidth})"));
    report.pageWidths.push(sizes);
    if (sizes.scroll > sizes.viewport + 1) report.failures.push({page: sizes});
  }
  await browser.viewport(1440);
  const cardCount = await browser.evaluate("catalog.cards.length");
  // Each batch uses the actual shared renderer, image decoders and font fitter.
  // Run twice with the same nodes in the two languages to cover re-rendering.
  for (const width of smoke ? [] : [180, 288]) {
    for (let offset = 0; offset < cardCount; offset += 48) {
      if (process.env.DOMINION_LAYOUT_PROGRESS) console.error(browser.version, width, offset);
      const result = JSON.parse(await browser.evaluate(`(async () => {
        const host = document.createElement('div');
        host.id = 'layout-fixture'; host.style.cssText = 'position:absolute;left:0;top:0;z-index:20;display:grid;grid-template-columns:repeat(6,${width}px);gap:8px;background:var(--paper)';
        document.body.append(host);
        const cards = catalog.cards.slice(${offset}, ${offset + 48});
        const views = cards.map((card, i) => {
          const panel = document.createElement('div'); panel.id = 'layout-${width}-' + i; panel.className = 'result-card-visual';
          host.append(panel); return {card, panel};
        });
        const failures = []; let checks = 0;
        const bounded = (promise, label) => {
          let timer;
          return Promise.race([promise,new Promise((_,reject)=>timer=setTimeout(()=>reject(new Error(label+' timed out in batch ${offset}/${width}')),10000))]).finally(()=>clearTimeout(timer));
        };
        const frame = () => bounded(new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))), 'animation frame');
        for (const language of ['de', 'en']) {
          uiLanguage = language;
          for (const view of views) updateCardVisual(view);
          await bounded(Promise.all([...host.querySelectorAll('img[src]')].map(image => image.decode().catch(() => {}))), 'image decode');
          await frame();
          for (const view of views) {
            const c = view.content; checks++;
            if (c.transcript.hidden || c.transcript.open || !c.textRules.childElementCount) failures.push({id:view.card.id, language, kind:'transcript'});
            if (c.summary.textContent !== (language === 'de' ? 'Effekt als Text' : 'Effect as text')) failures.push({id:view.card.id,language,kind:'transcript-language'});
            if (c.rules.textContent !== c.textRules.textContent || c.rules.querySelectorAll('img').length !== c.textRules.querySelectorAll('img').length) failures.push({id:view.card.id,language,kind:'transcript-content'});
            c.transcript.open = true;
            if (!c.textRules.getBoundingClientRect().height) failures.push({id:view.card.id,language,kind:'transcript-toggle'});
            c.transcript.open = false;
            if (!c.face.hidden) {
              const box = c.face.getBoundingClientRect(), expected = c.face.classList.contains('preview-face-landscape') ? 524/344 : 344/524;
              if (Math.abs(box.width/box.height - expected) > .012) failures.push({id:view.card.id, language, kind:'ratio',w:box.width,h:box.height});
              for (const [kind, node] of [['title',c.heading],['rules',c.rules],['types',c.typeLine]]) {
                const parent = kind === 'rules' ? c.ruleBody : node;
                const padding = getComputedStyle(parent);
                const width = parent.clientWidth - (kind === 'rules' ? parseFloat(padding.paddingLeft)+parseFloat(padding.paddingRight) : 0);
                const height = parent.clientHeight - (kind === 'rules' ? parseFloat(padding.paddingTop)+parseFloat(padding.paddingBottom) : 0);
                if (node.scrollWidth > width + 2 || node.scrollHeight > height + 2) failures.push({id:view.card.id,language,kind,w:[node.scrollWidth,width],h:[node.scrollHeight,height]});
              }
              const footer = c.cost.parentElement.getBoundingClientRect(), coin = c.cost.getBoundingClientRect(), title = c.heading.getBoundingClientRect();
              if (coin.left < box.left-2 || coin.right > box.right+2 || coin.top < footer.top-2 || coin.bottom > box.bottom+2) failures.push({id:view.card.id,language,kind:'cost-bounds',cost:[coin.left,coin.top,coin.right,coin.bottom],footer:[footer.left,footer.top,footer.right,footer.bottom],face:[box.left,box.top,box.right,box.bottom]});
              if (title.left < box.left-2 || title.right > box.right+2 || title.top < box.top-2 || title.bottom > box.bottom+2) failures.push({id:view.card.id,language,kind:'title-bounds'});
              const type = c.typeLine.getBoundingClientRect(), emblem = c.cost.parentElement.lastElementChild.getBoundingClientRect();
              if (type.left < coin.right-2 || type.right > emblem.left+2) failures.push({id:view.card.id,language,kind:'footer-overlap'});
            }
          }
        }
        if (${offset} === 0) {
          host.style.gridTemplateColumns = 'repeat(6,${width === 180 ? 288 : 180}px)';
          await frame(); await frame();
          for (const view of views) {
            const c=view.content; if(c.face.hidden) continue;
            const style=getComputedStyle(c.ruleBody);
            const w=c.ruleBody.clientWidth-parseFloat(style.paddingLeft)-parseFloat(style.paddingRight);
            const h=c.ruleBody.clientHeight-parseFloat(style.paddingTop)-parseFloat(style.paddingBottom);
            if(c.rules.scrollWidth>w+2||c.rules.scrollHeight>h+2) failures.push({id:view.card.id,kind:'automatic-resize-fit'});
          }
        }
        // Missing images must still expose both the composed face and transcript.
        const missing = views.find(view => view.card.id === 'cellar') ?? views[0];
        missing.rendered = null;
        const old = imageData.images[missing.card.id], oldFull = imageData.cards?.[uiLanguage]?.[missing.card.id];
        imageData.images[missing.card.id] = '/missing-layout-test.png';
        if (oldFull) imageData.cards[uiLanguage][missing.card.id] = '/missing-layout-test.png';
        updateCardVisual(missing);
        await bounded(Promise.all([...missing.panel.querySelectorAll('img[src]')].map(image => image.decode().catch(() => {}))), 'missing-image decode');
        await frame();
        if (missing.content.face.hidden || missing.content.transcript.hidden) failures.push({id:missing.card.id,kind:'missing-image'});
        imageData.images[missing.card.id] = old;
        if (oldFull) imageData.cards[uiLanguage][missing.card.id] = oldFull;
        releaseCardVisuals(host); releaseTooltipPins(host); host.remove();
        return JSON.stringify({checks, failures});
      })()`));
      report.checks += result.checks; report.failures.push(...result.failures.map(f => ({width, ...f})));
    }
  }
  report.interactions = JSON.parse(await browser.evaluate(`(async () => {
    const fail=[]; const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));
    const host=document.createElement('div'); host.style.cssText='position:fixed;top:10px;left:10px;width:288px;z-index:30';document.body.append(host);
    uiLanguage='de';
    const button=addCardPreview(host,cardById.get('page'),'layout-hover');
    button.dispatchEvent(new PointerEvent('pointerenter',{pointerType:'mouse'}));
    const parent=[...previews].find(p=>p.button===button);
    if(parent.panel.hidden)fail.push('hover-open');
    parent.panel.click();button.dispatchEvent(new PointerEvent('pointerleave',{pointerType:'mouse'}));
    await wait(350);
    if(parent.panel.hidden||!parent.panel.hasAttribute('data-tooltip-pinned'))fail.push('pin-hover-leave');
    parent.content.relatedSection.toggle.click();
    const nested=[...previews].find(p=>parent.content.relatedSection.body.contains(p.button)&&p.card.id==='champion');
    if(!nested)fail.push('related-champion');
    else {nested.button.click();if(nested.panel.hidden||parent.panel.hidden||!pinnedTooltips.has(parent.panel))fail.push('nested-pin');}
    document.dispatchEvent(new KeyboardEvent('keydown',{key:'Escape',bubbles:true}));
    if(!parent.panel.hidden||(nested&&!nested.panel.hidden))fail.push('escape-close');
    button.focus();
    if(parent.panel.hidden)fail.push('focus-open');
    button.click();document.body.click();
    if(!parent.panel.hidden)fail.push('outside-close');
    const item=document.createElement('div');host.append(item);
    const referenceButton=addCardPreview(item,cardById.get('moneylender'),'layout-reference');referenceButton.click();
    const referencePreview=[...previews].find(p=>p.button===referenceButton);
    referencePreview.content.transcript.open=true;
    const reference=referencePreview.content.textRules.querySelector('.card-reference');
    if(!reference)fail.push('transcript-reference');
    else {
      reference.dispatchEvent(new KeyboardEvent('keydown',{key:'Enter',bubbles:true,cancelable:true}));
      const hint=reference.querySelector('.card-reference-hint');
      if(!pinnedTooltips.has(hint)||!pinnedTooltips.has(referencePreview.panel))fail.push('nested-reference-pin');
    }
    document.body.click();
    if(!referencePreview.panel.hidden)fail.push('nested-reference-outside');
    const ids=[...host.querySelectorAll('[id]')].map(node=>node.id);
    if(new Set(ids).size!==ids.length)fail.push('duplicate-preview-ids');
    releaseCardVisuals(host);releaseTooltipPins(host);
    for(const preview of [...previews])if(host.contains(preview.button)){closeCardPreview(preview);previews.delete(preview);}
    host.remove();
    return JSON.stringify(fail);
  })()`));
  report.failures.push(...report.interactions.map(kind=>({kind})));
  const setup = JSON.parse(await browser.evaluate(`(async () => {
    uiLanguage = 'de';
    const result = {main:['duplicate','vault','cellar','mint','market','moat','village','smithy','chapel','workshop'],landscapes:['advance','way_of_the_horse'],components:['platinum','potion','curse','champion','key'],assignments:[],material:[],needsSteamCheck:[],randomExpansions:[]};
    showDraw(result);
    await Promise.all([...document.querySelectorAll('#cards img[src], #setup-result img[src]')].map(image => image.decode().catch(() => {})));
    await new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    const counts = {visuals:setupResult.querySelectorAll('.result-card-visual').length,expected:result.landscapes.length+result.components.length};
    cardsList.scrollIntoView();
    return JSON.stringify(counts);
  })()`));
  if (setup.visuals !== setup.expected) report.failures.push({kind:"setup-visuals",...setup});
  for (const width of [390, 900, 1920, 2560]) {
    await browser.viewport(width); await delay(80);
    const result = JSON.parse(await browser.evaluate(`JSON.stringify({overflow:document.documentElement.scrollWidth>innerWidth+1,heights:[...cardsList.children].map(item=>{const r=item.getBoundingClientRect();return{top:r.top,height:r.height}})})`));
    if (result.overflow) report.failures.push({kind:"result-page-overflow",width});
    const rows = new Map();
    for (const item of result.heights) {
      const key = Math.round(item.top);
      if (rows.has(key) && Math.abs(rows.get(key)-item.height)>1) report.failures.push({kind:"result-row-height",width});
      rows.set(key,item.height);
    }
  }
  await browser.viewport(1920);
  await browser.evaluate("for(const id of ['duplicate','vault','cellar','mint'].reverse()){const card=[...cardsList.children].find(item=>item.querySelector('[id$=\"-' + id + '\"]'));if(card)cardsList.prepend(card)};cardsList.scrollIntoView()");
  await delay(80);
  const screenshot = path.join(os.tmpdir(), "dominion-card-layout-" + report.browser.split(/[ /]/)[0] + ".png");
  fs.writeFileSync(screenshot, Buffer.from(await browser.screenshot(), "base64"));
  report.screenshot = screenshot;
  await browser.evaluate("setupResult.scrollIntoView()");
  const setupScreenshot = screenshot.replace(".png", "-setup.png");
  fs.writeFileSync(setupScreenshot, Buffer.from(await browser.screenshot(), "base64"));
  report.setupScreenshot = setupScreenshot;
  await browser.evaluate("themeSelect.value='light';themeSelect.dispatchEvent(new Event('change'));languageSelect.value='en';languageSelect.dispatchEvent(new Event('change'))");
  for (const width of [390, 1920]) {
    await browser.viewport(width); await delay(100);
    if (await browser.evaluate("document.documentElement.scrollWidth>innerWidth+1")) report.failures.push({kind:"light-english-page-overflow",width});
  }
  await browser.evaluate("cardsList.scrollIntoView()");
  const lightScreenshot = screenshot.replace(".png", "-light-en.png");
  fs.writeFileSync(lightScreenshot, Buffer.from(await browser.screenshot(), "base64"));
  report.lightScreenshot = lightScreenshot;
  return report;
}

(async () => {
  const server = http.createServer((req, res) => {
    const pathname = decodeURIComponent(new URL(req.url, "http://localhost").pathname);
    const file = path.resolve(root, "." + pathname);
    if (!file.startsWith(root + path.sep)) { res.writeHead(403); res.end(); return; }
    const types = {".html": "text/html; charset=utf-8", ".png": "image/png", ".webp": "image/webp", ".jpg": "image/jpeg", ".json": "application/json"};
    fs.readFile(file, (error, data) => { if(error) {res.writeHead(404);res.end();} else {res.setHeader("Content-Type",types[path.extname(file)]??"application/octet-stream");res.end(data);} });
  });
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  const url = `http://127.0.0.1:${server.address().port}/Dominion-Generator.html`;
  try {
    const reports = [];
    for (const name of requested === "both" ? ["edge", "firefox"] : [requested]) {
      const browser = await launch(name, url);
      try { const report = await exercise(browser); reports.push(report); console.log(JSON.stringify(report)); }
      finally { await browser.close(); }
    }
    if (reports.some(report => report.failures.length)) process.exitCode = 1;
  } finally { await new Promise(resolve => server.close(resolve)); }
})().catch(error => { console.error(error); process.exitCode = 1; });

