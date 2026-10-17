"""X-TOOLS probe: how the IP token list behaves under candidate normalisations.
Usage: python3 -I X-TOOLS_probe.py <ip_tokens.txt> <prose_vocab.tsv>"""
import csv
import re
import sys
import unicodedata

toks = [l.strip() for l in open(sys.argv[1], encoding='utf-8') if l.strip()]
vocab = {r['word']: int(r['lower_count']) for r in csv.DictReader(open(sys.argv[2], encoding='utf-8'), delimiter='\t')}


def fold(s):
  return unicodedata.normalize('NFKD', s).encode('ascii', 'ignore').decode('ascii').lower()


def deleted(s):
  return ' '.join(w for w in (re.sub(r'[^a-z0-9]', '', c) for c in fold(s).split()) if w)


def spaced(s):
  return ' '.join(re.sub(r'[^a-z0-9]+', ' ', fold(s)).split())


eng = lambda p: all(vocab.get(w, 0) >= 3 for w in p.split())
gen_del = [t for t in toks if deleted(t) != t and eng(deleted(t))]
print('generic after deletion:', gen_del)
print('all-English entries without punctuation:', [t for t in toks if deleted(t) == t and eng(t)])
print('max words spaced:', max(len(spaced(t).split()) for t in toks), [t for t in toks if len(spaced(t).split()) > 5][:5])
print('max words deleted:', max(len(deleted(t).split()) for t in toks))
d = {}
for t in toks:
  d.setdefault(deleted(t), []).append(t)
print('deletion collisions:', {k: v for k, v in d.items() if len(v) > 1})
print('non-fixed-point under fold:', [t for t in toks if fold(t) != t])
