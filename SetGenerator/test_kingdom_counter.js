"use strict";
const assert = require("node:assert/strict");
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm");
const html = fs.readFileSync(path.join(__dirname, "Dominion-Generator.html"), "utf8");
const start = html.indexOf("function setupKingdomCounter() {");
const source = html.slice(start, html.indexOf("\n}", start) + 2);
function fixture(href = "https://serpens66.github.io/Dominion/") {
  const state = { images: [], requests: [], pending: [], labels: [], listeners: [], language: "de" };
  const total = { hidden: true }, slot = { replaceChildren(image) { this.image = image; } };
  const context = {
    location: new URL(href), URL, URLSearchParams, Date, AbortSignal,
    Image: class { constructor() { state.images.push(this); } },
    document: { getElementById: id => id === "kingdom-total" ? total : slot },
    drawButton: { addEventListener: (event, handler) => { assert.equal(event, "click"); state.listeners.push(handler); } },
    interfaceLabels: state.labels, tr: (de, en) => state.language === "en" ? en : de,
    fetch: (url, options) => {
      state.requests.push({ url: new URL(url), options });
      return new Promise((resolve, reject) => state.pending.push({ resolve, reject }));
    },
  };
  vm.runInNewContext(source + "\nsetupKingdomCounter();", context);
  return Object.assign(state, { total, slot });
}
(async () => {
  for (const href of ["file:///D:/Dominion-Generator.html", "http://localhost:8000/Dominion/", "https://example.com/Dominion/", "https://serpens66.github.io/Other/", "http://serpens66.github.io/Dominion/"]) {
    const f = fixture(href);
    assert.equal(f.images.length + f.requests.length + f.listeners.length, 0, href);
  }
  assert.equal(fixture("https://serpens66.github.io/Dominion/index.html").images.length, 1);
  const f = fixture();
  assert.equal(f.requests.length, 0, "loading does not increment");
  assert.equal(f.total.hidden, true);
  const initial = new URL(f.images[0].src);
  assert.equal(initial.searchParams.get("read"), "true");
  assert.equal(initial.searchParams.has("unique"), false, "even unique=false would enable cookie mode on this server");
  f.images[0].onerror();
  assert.equal(f.total.hidden, true, "failed initial image stays hidden");
  f.listeners[0](); f.listeners[0]();
  assert.equal(f.requests.length, 2, "repeated clicks each count once");
  for (const { url, options } of f.requests) {
    assert.equal(url.pathname, "/count");
    assert.equal(url.searchParams.has("read"), false);
    assert.equal(url.searchParams.has("unique"), false);
    assert.equal(options.credentials, "omit");
    assert.equal(options.mode, "no-cors");
    assert.equal(options.cache, "no-store");
    assert.equal(options.referrerPolicy, "no-referrer");
  }
  f.pending[0].resolve({}); f.pending[1].resolve({});
  await new Promise(setImmediate);
  assert.equal(f.images.length, 3);
  f.images[2].onload();
  assert.equal(f.total.hidden, false);
  const current = f.slot.image;
  f.images[1].onload();
  assert.equal(f.slot.image, current, "older reads cannot replace the latest image");
  f.language = "en"; f.labels.forEach(render => render());
  assert.equal(current.alt, "Number of draws");
  assert.equal(f.requests.length, 2, "language changes do not increment");
  f.listeners[0](); f.pending[2].reject(new Error("offline"));
  await new Promise(setImmediate);
  assert.equal(f.slot.image, current);
  assert.equal(f.requests.length, 3, "failed increments are not retried");
  f.listeners[0](); f.pending[3].resolve({});
  await new Promise(setImmediate);
  f.images[3].onerror();
  assert.equal(f.slot.image, current, "failed refresh preserves the displayed count");
  console.log("Kingdom counter: production-only requests, read-only loading, repeated clicks, language changes, stale responses and failures passed.");
})().catch(error => { console.error(error); process.exitCode = 1; });
