# DesignModels

Quantitative models of the design. They are development tools: nothing here ships or links into the executable
(`AGENTS.md` R14). Each model is written from the design documents alone and states which document it implements.

## TerrainGenerator.py

An independent implementation of terrain height-field generation, written from the terrain specification
(`Design/architecture/components/terrain/`) alone. It reproduces the test values of that specification: the vector
sets of `height_field_generation.md` §7.2, which tests T1 to T3 and T7 to T13 of the terrain `README.md` §7.1 use. It
is the conformance generator that the specification names: the expected values of a new vector set are produced with
it, never taken from a map.

```
python3 Tools/DesignModels/TerrainGenerator.py            # check: every test value, exact equality
python3 Tools/DesignModels/TerrainGenerator.py traceA     # set A: terrain values, tile 2 events, derived values
python3 Tools/DesignModels/TerrainGenerator.py traceM1    # set A, tile 0 (method 1): guide pins and first calls
python3 Tools/DesignModels/TerrainGenerator.py traceB     # set B (method 2): first calls, highest sample
python3 Tools/DesignModels/TerrainGenerator.py smooth     # set C: one smooth evaluation
python3 Tools/DesignModels/TerrainGenerator.py flatten    # set D: the five flatten areas in order
python3 Tools/DesignModels/TerrainGenerator.py full       # traceA, then normals and bilinear reads
python3 Tools/DesignModels/TerrainGenerator.py log2       # ceilLog2 for every integer from 1 to 69 999
```

`check` prints one line per test and exits with status 1 when any value differs. Values are printed with `repr()`,
which gives the shortest text that parses back to the same binary64 value. Python 3 with the standard library only;
no input files.

## Adding a model

The design phase adds its quantitative models here: economy and pacing curves, population and spawn models,
balance tables and any other model whose numbers a design document states. Each model is one Python file named in the
`AGENTS.md` style (`PacingModel.py`), LF line endings, 2-space indent and 120 columns (`.editorconfig`), with a
docstring naming the design document it implements and a `check` mode that reproduces the numbers that document
states. Add it to this file.
