// MermaidCheck.js: gate 4 of CheckDesign.py. Parses every ```mermaid block of every Markdown file under a design
// root (recursively) with mermaid.parse() in headless Chromium, using the pinned Mermaid build of package.json.
//
// Usage: node Tools/DesignCheck/MermaidCheck.js <design root> <mermaid.min.js> [<results.json>]
//
// Playwright is resolved from Tools/DesignCheck/node_modules (npm ci --prefix Tools/DesignCheck), and Playwright
// locates Chromium itself, honouring PLAYWRIGHT_BROWSERS_PATH (npx --prefix Tools/DesignCheck playwright install
// chromium puts it where Playwright looks by default).
//
// Output: one line per failing block, "FAIL <file>:<line> block#<n> [<first line>] <error>", then
// "blocks=<n> failed=<k>". Exit status: 0 all blocks parse, 1 at least one block fails, 2 usage or setup error
// (Playwright, Chromium or the Mermaid build missing), with an "ERROR <reason>" line.
'use strict';

const fs = require('fs');
const path = require('path');

const [designRoot, mermaidJs, resultsJson] = process.argv.slice(2);
if (!designRoot || !mermaidJs) {
  console.log('ERROR usage: node MermaidCheck.js <design root> <mermaid.min.js> [<results.json>]');
  process.exit(2);
}
if (!fs.existsSync(mermaidJs)) {
  console.log(`ERROR Mermaid build not found: ${mermaidJs} (run: npm ci --prefix Tools/DesignCheck)`);
  process.exit(2);
}

let chromium;
try {
  ({ chromium } = require('playwright'));
} catch (e) {
  console.log('ERROR Playwright not found beside MermaidCheck.js (run: npm ci --prefix Tools/DesignCheck)');
  process.exit(2);
}

function walk(dir) {
  const out = [];
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    if (entry.name === '.git' || entry.name === 'node_modules') {
      continue;
    }
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      out.push(...walk(full));
    } else if (entry.name.endsWith('.md')) {
      out.push(full);
    }
  }
  return out.sort();
}

const blocks = [];
for (const file of walk(designRoot)) {
  const text = fs.readFileSync(file, 'utf8');
  const fence = /^[ \t]*```mermaid[ \t]*\r?\n([\s\S]*?)^[ \t]*```/gm;
  let match;
  let index = 0;
  while ((match = fence.exec(text)) !== null) {
    const line = text.slice(0, match.index).split('\n').length;
    const rel = path.relative(designRoot, file).split(path.sep).join('/');
    blocks.push({ file: rel, index: index++, line, code: match[1] });
  }
}

(async () => {
  let browser;
  try {
    browser = await chromium.launch();
  } catch (e) {
    const reason = String((e && e.message) || e).split('\n')[0];
    console.log(`ERROR Chromium did not start through Playwright: ${reason} ` +
      '(run: npx --prefix Tools/DesignCheck playwright install chromium, or set PLAYWRIGHT_BROWSERS_PATH)');
    process.exit(2);
  }
  const page = await browser.newPage();
  await page.setContent('<html><body></body></html>');
  await page.addScriptTag({ path: mermaidJs });
  await page.evaluate(() => mermaid.initialize({ startOnLoad: false, maxTextSize: 10000000, maxEdges: 100000 }));
  const results = [];
  for (const block of blocks) {
    const error = await page.evaluate(async (code) => {
      try {
        await mermaid.parse(code);
        return null;
      } catch (e) {
        return String((e && e.message) || e).slice(0, 400);
      }
    }, block.code);
    const head = block.code.split('\n')[0].trim();
    results.push({ file: block.file, index: block.index, line: block.line, ok: error === null, error, head });
  }
  await browser.close();
  const failed = results.filter((r) => !r.ok);
  for (const r of failed) {
    console.log(`FAIL ${r.file}:${r.line} block#${r.index} [${r.head}] ${r.error.replace(/\s+/g, ' ')}`);
  }
  console.log(`blocks=${results.length} failed=${failed.length}`);
  if (resultsJson) {
    fs.writeFileSync(resultsJson, JSON.stringify(results, null, 1));
  }
  process.exit(failed.length ? 1 : 0);
})();
