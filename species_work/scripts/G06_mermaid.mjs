// Usage: node G06_mermaid.mjs <mermaid.core.mjs path> <md files...>
import fs from 'node:fs';
const [,, modPath, ...files] = process.argv;
const purifyPath = modPath.replace(/mermaid\/dist\/mermaid\.core\.mjs$/, 'dompurify/dist/purify.es.mjs');
const { default: DOMPurify } = await import(purifyPath);
if (!DOMPurify.addHook) { DOMPurify.addHook = () => {}; DOMPurify.removeHook = () => {}; DOMPurify.removeHooks = () => {}; DOMPurify.sanitize = (s) => s; }
const { default: mermaid } = await import(modPath);
mermaid.initialize({ startOnLoad: false });
let bad = 0, n = 0;
for (const f of files) {
  const t = fs.readFileSync(f, 'utf8');
  const re = /```mermaid\n([\s\S]*?)```/g;
  let m;
  while ((m = re.exec(t))) {
    n++;
    try { await mermaid.parse(m[1]); }
    catch (e) { bad++; console.log('FAIL', f, String(e.message || e).slice(0, 300)); }
  }
}
console.log(`parsed ${n} blocks, ${bad} failures`);
