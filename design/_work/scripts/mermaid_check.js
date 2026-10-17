// Validate every ```mermaid block in the design documents with mermaid.parse() inside headless Chromium.
// Usage: NODE_PATH=<dir containing playwright> node mermaid_check.js <DESIGN_DIR> <mermaid.min.js> [out.json]
// Prints one line per failing block (file, block index, first line, error) and a summary; exit 1 if any fail.
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const [designDir, mermaidJs, outJson] = process.argv.slice(2);
const docs = fs.readdirSync(designDir).filter(f => f.endsWith('.md')).sort();
const blocks = [];
for (const f of docs) {
  const text = fs.readFileSync(path.join(designDir, f), 'utf8');
  const re = /```mermaid\s*\n([\s\S]*?)```/g;
  let m, i = 0;
  while ((m = re.exec(text)) !== null) {
    const line = text.slice(0, m.index).split('\n').length;
    blocks.push({ file: f, index: i++, line, code: m[1] });
  }
}

(async () => {
  const browser = await chromium.launch({ executablePath: '/opt/pw-browsers/chromium-1194/chrome-linux/chrome' });
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
  const perFile = {};
  for (const r of results) { perFile[r.file] = perFile[r.file] || [0, 0]; perFile[r.file][0]++; if (!r.ok) perFile[r.file][1]++; }
  console.log(`blocks=${results.length} failed=${bad.length}`);
  for (const [f, [n, k]] of Object.entries(perFile)) console.log(`  ${f}: ${n} blocks, ${k} failed`);
  if (outJson) fs.writeFileSync(outJson, JSON.stringify(results, null, 1));
  process.exit(bad.length ? 1 : 0);
})();
