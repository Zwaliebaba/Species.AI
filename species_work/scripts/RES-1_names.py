"""RES-1 helper: list PascalCase and UPPER_CASE identifiers of the C++ sketches in the given
Markdown files that CONVENTIONS.md does not mention.

Usage: python3 -I RES-1_names.py <CONVENTIONS.md> <file.md> [<file.md> ...]
"""
import re
import sys

SKIP = {'std', 'const', 'void', 'bool', 'int', 'float', 'double', 'auto', 'class', 'struct', 'enum', 'namespace', 'public',
        'private', 'protected', 'virtual', 'override', 'static', 'inline', 'constexpr', 'noexcept', 'return', 'false',
        'true', 'nullptr', 'Engine', 'Species', 'ID3D12Device', 'ID3D12Resource', 'DXGI_FORMAT', 'HRESULT'}


def main(conv, files):
  conv_text = open(conv, encoding='utf-8').read()
  for path in files:
    text = open(path, encoding='utf-8').read()
    missing = set()
    for block in re.findall(r'```cpp\n(.*?)```', text, re.S):
      block = re.sub(r'//[^\n]*', '', block)
      for name in re.findall(r'\b([A-Z][A-Za-z0-9_]*)\b', block):
        if name in SKIP or len(name) < 3:
          continue
        if not re.search(r'(?<![A-Za-z0-9_])' + re.escape(name) + r'(?![A-Za-z0-9_])', conv_text):
          missing.add(name)
    if missing:
      print(f'{path}: {", ".join(sorted(missing))}')


if __name__ == '__main__':
  main(sys.argv[1], sys.argv[2:])
