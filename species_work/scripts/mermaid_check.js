// Validate every ```mermaid block under a design folder (recursive) with mermaid.parse() in headless Chromium.
// Usage: NODE_PATH=<dir containing playwright> node mermaid_check.js <DESIGN_DIR> <mermaid.min.js> [out.json]
// Chromium is located through PLAYWRIGHT_BROWSERS_PATH (default /opt/pw-browsers); use a pinned mermaid 11 build.
// Prints one line per failing block (FAIL file:line block#n [first line] error), then blocks=<n> failed=<k>.
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const [designDir, mermaidJs, outJson] = process.argv.slice(2);
if (!designDir || !mermaidJs) { console.error('usage: node mermaid_check.js <DESIGN_DIR> <mermaid.min.js> [out.json]'); process.exit(2); }

function walk(dir) {
  const out = [];
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    if (e.name === '.git' || e.name === 'node_modules') continue;
    const p = path.join(dir, e.name);
    if (e.isDirectory()) out.push(...walk(p));
    else if (e.name.endsWith('.md')) out.push(p);
  }
  return out.sort();
}

function findChromium() {
  const base = process.env.PLAYWRIGHT_BROWSERS_PATH || '/opt/pw-browsers';
  const dirs = fs.existsSync(base) ? fs.readdirSync(base).filter(d => /^chromium-\d+$/.test(d)).sort() : [];
  for (const d of dirs.reverse()) {
    for (const rel of ['chrome-linux/chrome', 'chrome-linux64/chrome', 'chrome-win/chrome.exe']) {
      const p = path.join(base, d, rel);
      if (fs.existsSync(p)) return p;
    }
  }
  return undefined; // let playwright resolve it
}

const blocks = [];
for (const file of walk(designDir)) {
  const text = fs.readFileSync(file, 'utf8');
  const re = /^[ \t]*```mermaid[ \t]*\r?\n([\s\S]*?)^[ \t]*```/gm;
  let m, i = 0;
  while ((m = re.exec(text)) !== null) {
    const line = text.slice(0, m.index).split('\n').length;
    blocks.push({ file: path.relative(designDir, file).split(path.sep).join('/'), index: i++, line, code: m[1] });
  }
}

(async () => {
  const browser = await chromium.launch({ executablePath: findChromium() });
  const page = await browser.newPage();
  await page.setContent('<html><body></body></html>');
  await page.addScriptTag({ path: mermaidJs });
  await page.evaluate(() => mermaid.initialize({ startOnLoad: false, maxTextSize: 10000000, maxEdges: 100000 }));
  const results = [];
  for (const b of blocks) {
    const err = await page.evaluate(async (code) => {
      try { await mermaid.parse(code); return null; } catch (e) { return String(e && e.message || e).slice(0, 400); }
    }, b.code);
    results.push({ file: b.file, index: b.index, line: b.line, ok: err === null, error: err, head: b.code.split('\n')[0].trim() });
  }
  await browser.close();
  const bad = results.filter(r => !r.ok);
  for (const r of bad) console.log(`FAIL ${r.file}:${r.line} block#${r.index} [${r.head}] ${r.error.replace(/\s+/g, ' ')}`);
  console.log(`blocks=${results.length} failed=${bad.length}`);
  if (outJson) fs.writeFileSync(outJson, JSON.stringify(results, null, 1));
  process.exit(bad.length ? 1 : 0);
})();
