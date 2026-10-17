import sys, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8')
edits = [
 ("as (remaining delay ÷ 300) goes from 1 to 0, and their opacity is (occupied ports ÷ 5).",
  "as (remaining delay / 300) goes from 1 to 0, and their opacity is (occupied ports / 5)."),
 ("(H = 1 + h ÷ 100 when the bonus applies)", "(H = 1 + h / 100 when the bonus applies)"),
 ("In effect, H = 1 + h ÷ 100 (1.9 at the default)", "In effect, H = 1 + h / 100 (1.9 at the default)"),
 ("and H = 1 + (h ÷ 100) × factor;", "and H = 1 + (h / 100) × factor;"),
 ("1, with probability 1 ÷ (`ref.econ.essence_grove.drop_base`", "1, with probability 1 / (`ref.econ.essence_grove.drop_base`"),
 ("drawn as (120 + U[−30, 30)) ÷ 3 s);", "drawn as (120 + U[−30, 30)) / 3 s);"),
 ("with an eat chance of its size ÷ `ref.econ.centipede.eat_chance_divisor`", "with an eat chance of its size / `ref.econ.centipede.eat_chance_divisor`"),
 ("uniform; (120 + U[−30, 30)) ÷ 3 s = 30–50 s;", "uniform; (120 + U[−30, 30)) / 3 s = 30–50 s;"),
 ("per-tick probability 1 ÷ (51 − drop rate)", "per-tick probability 1 / (51 − drop rate)"),
 ("eat chance = size ÷ 2 |", "eat chance = size / 2 |"),
 ("takes about (1000 − 45) ÷ 3.67 ≈ 260 s.", "takes about (1000 − 45) / 3.67 ≈ 260 s."),
 ("per (2 × 500 ÷ 28 ≈ 36 s of travel", "per (2 × 500 / 28 ≈ 36 s of travel"),
 ("a timer of n ticks lasts n ÷ (10 × speed) real seconds", "a timer of n ticks lasts n / (10 × speed) real seconds"),
 ("- **Integer division.** `÷` between integers truncates toward zero (DEC-241); `⌊x⌋` is the floor.",
  "- **Division.** `÷` is integer division, which truncates toward zero (DEC-241); `/` is exact division; `⌊x⌋` is the floor."),
 ("at least ⌊(flagCount + 1) ÷ 2⌋ Flag posts", "at least (flagCount + 1) ÷ 2 Flag posts"),
]
for old, new in edits:
    n = t.count(old)
    if n != 1:
        print('MISS', n, old[:70]); continue
    t = t.replace(old, new)
p.write_text(t, encoding='utf-8')
print('done')
