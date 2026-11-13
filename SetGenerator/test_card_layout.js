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

async function exercise(browser) {
  for (let i = 0; i < 100; i++) {
    if (await browser.evaluate("typeof fitCardFace === 'function' && typeof catalog !== 'undefined'")) break;
    await delay(100);
  }
  const report = {browser: browser.version, checks: 0, failures: [], pageWidths: []};
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

