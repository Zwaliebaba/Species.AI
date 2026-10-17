# Usage: python3 -I P12_glstate.py <file> <FunctionQualifiedName> ...
# Prints unique GL state calls (glEnable/glDisable/glBlendFunc/glDepthMask/glAlphaFunc/glLineWidth/SetupMatrices/BeginText2D/GetTexture) found in the function body (brace matched).
import sys, re
path=sys.argv[1]; src=open(path,encoding='latin-1').read()
for fn in sys.argv[2:]:
    m=re.search(r'\n[^\n;]*\b'+re.escape(fn)+r'\s*\([^)]*\)\s*(const)?\s*\{', src)
    if not m: print(fn,'NOT FOUND'); continue
    i=m.end()-1; depth=0; j=i
    while j<len(src):
        if src[j]=='{': depth+=1
        elif src[j]=='}':
            depth-=1
            if depth==0: break
        j+=1
    body=src[i:j]
    calls=re.findall(r'(glEnable\s*\([^)]*\)|glDisable\s*\([^)]*\)|glBlendFunc\s*\([^)]*\)|glDepthMask\s*\([^)]*\)|glAlphaFunc\s*\([^)]*\)|glLineWidth\s*\([^)]*\)|glPolygonMode\s*\([^)]*\)|glShadeModel\s*\([^)]*\)|SetupMatricesFor[23]D\s*\([^)]*\)|BeginText2D\s*\(\)|EndText2D\s*\(\)|SetupProjectionMatrix|GetTexture\w*\s*\(\s*"[^"]*")',body)
    seen=[]; 
    for c in calls:
        c=re.sub(r'\s+',' ',c)
        if c not in seen: seen.append(c)
    print('==',fn,'(%d lines)'%body.count('\n')); print('   '+' | '.join(seen))
