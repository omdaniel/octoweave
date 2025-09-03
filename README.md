# OctoWeave

A scaffold for building a chunked OctoMap → p4est (octree path) pipeline with:
- n×n×n chunking and per-chunk OctoMap insertion (parallelizable)
- bottom-up union of children (probabilistic)
- export to a p4est-based forest (n³ roots via brick connectivity)
- a CLI visualization tool for OctoMap levels (implemented: PGM slice and SVG histogram)
- stepwise agent prompts (Codex CLI) to implement & test modules in phases

## Motivation & Problem Statement

Most mapping stacks face a trade‑off between fidelity, throughput, and downstream
mesh/analysis friendliness:

- High‑fidelity occupancy maps (e.g., OctoMap) are great for probabilistic fusion,
  but become heavy at deep resolutions and awkward to distribute across cores/nodes.
- Distributed/AMR meshes (e.g., p4est) are excellent for parallelism and
  numeric workloads, but don’t natively encode Bayesian fusion or sensor semantics.

OctoWeave bridges these worlds by:

- Ingesting point clouds into per‑chunk OctoMaps (easy to parallelize) at a chosen
  resolution, with configurable sensor model (hit/miss/clamp, ray freespace).
- Performing bottom‑up probabilistic rollups to a compact hierarchy with well‑defined
  thresholds for leaf vs. internal nodes.
- Emitting a p4est forest with n³ root trees (brick layout) and per‑quadrant data
  (e.g., mean occupancy), using deterministic refinement policies.

The result is a fast, testable pipeline that produces an AMR structure ready for
large‑scale simulation, analysis, or further meshing — while retaining a principled
probabilistic foundation.

## When to Use OctoWeave

- Robotics & autonomy: fuse multiple scans into a globally consistent occupancy
  hierarchy, then export an AMR forest to plan or simulate at scale.
- Environmental mapping: dense regions (structures/vegetation) vs. sparse/empty
  perimeters — refine where it matters, stay coarse elsewhere.
- HPC pre‑processing: convert raw occupancy evidence into p4est for PDE/CFD/heat‑map
  solvers that expect adaptive trees and per‑cell data.
- Research prototyping: clear interfaces, stubs for optional deps, strong unit
  tests — easy to swap in real OctoMap/p4est when available.

## Why OctoMap + p4est

- OctoMap: mature Bayesian occupancy grid in an octree; sensor‑model friendly,
  robust log‑odds updates, freespace carving via ray insertion.
- p4est: scalable adaptive octree forests (n³ roots), proven in HPC; supports
  refinement, balancing, callbacks, and per‑quadrant user data; ideal for downstream numerics.

OctoWeave separates the concerns: fuse evidence where it’s natural (OctoMap), then
export to a data structure built for parallel mesh/AMR workflows (p4est).

## High‑Level Workflow

1) Chunking: Partition world AABB into n×n×n chunks; route points per chunk.
2) Per‑chunk build: Insert into OctoMaps (or stub), export leaf probabilities at a
   target emission resolution (with a safety cap on depth).
3) Hierarchy: Bottom‑up union; threshold with a child‑evidence guard; emit leaves vs.
   internals deterministically.
4) p4est mapping: Brick partition; split global keys into (tree, local); refine
   uniformly per tree to a policy‑driven target level; initialize per‑quadrant data
   (e.g., mean probability) and balance.
5) Visualization: Render slices, projections, and montages from CSV for inspection.

## Use‑Case Examples

- Dense center, sparse periphery: The MVN demo (python/examples/mvn_viz_demo.py)
  samples a mixture of multivariate normals and rejects points near the periphery
  to produce varied occupancy — illustrating banded colormaps (free/unknown/occupied),
  projections, montages, and overlays.
- Multi‑chunk sensor fusion: Feed each chunk with its point cloud, export WorkerOuts,
  then unify into a global hierarchy — leaves carry fused probabilities; internals
  indicate regions where children exist and exceed τ.
- AMR pre‑mesh: Choose a per‑tree refinement policy (uniform, quantile/bands by
  counts/means) to generate a p4est forest that’s both compact and simulation‑ready.

## Quick start
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build
```

## Build options
- `-DOCTOWEAVE_WITH_OCTOMAP=ON` to enable OctoMap integration
  - Homebrew hint: `-DCMAKE_PREFIX_PATH="/opt/homebrew/opt/octomap" -DCMAKE_BUILD_RPATH="/opt/homebrew/opt/octomap/lib"`
- `-DOCTOWEAVE_WITH_P4EST=ON` to enable p4est integration
  - MacPorts hint: `export PKG_CONFIG_PATH="/opt/local/lib/pkgconfig:${PKG_CONFIG_PATH}"` and `-DCMAKE_PREFIX_PATH="/opt/local" -DCMAKE_BUILD_RPATH="/opt/local/lib"`
- `-DOCTOWEAVE_BUILD_EXAMPLES=ON` to build C++ examples (default ON here)
- `-DOCTOWEAVE_BUILD_PYTHON=ON` to build a ctypes shared library and copy it into `python/octoweave_py/`

## Examples (C++)
- ex01: build a synthetic hierarchy, write CSV
  - `./build/ex01_synthetic_hierarchy` → writes `examples_out/leaves.csv`
- ex02: build p4est forests with different policies (real when p4est ON)
  - `./build/ex02_forest_policies`
- ex03: render a CSV slice and histogram via the in-lib viz
  - `./build/ex03_viz_export`
- ex04: read CSV → Hierarchy → build forest
  - `./build/ex04_csv_to_forest examples_out/leaves.csv`
- ex05: parallel per-chunk determinism demo
  - `./build/ex05_parallel_chunks`

## Python package
- Build the shared lib and copy into package directory:
  - `cmake -S . -B build -DOCTOWEAVE_BUILD_PYTHON=ON`
  - `cmake --build build -j`
- Import without install (development):
  - `python -c "import sys; sys.path.insert(0,'python'); from octoweave_py import OctoWeave, ChunkParams as P; print('ok')"`
- Build a wheel: `pip wheel ./python -w dist/`
- Install editable: `pip install -e ./python`

### Python API (ctypes)
- `ChunkParams(res, prob_hit, prob_miss, clamp_min, clamp_max, origin, max_range, lazy_eval, discretize, emit_res, max_depth_cap)`
- `OctoWeave.build_hierarchy_from_points(xyz_iterable, params, tau=0.5, p_unknown=0.5, base_depth=1)`
- `OctoWeave.write_csv(path)`
- `OctoWeave.build_forest_uniform(n, level)` (real forest if p4est ON, stub otherwise)
- `OctoWeave.compute_levels_by_leafcount_quantiles(n, q_lo, q_hi, Llow, Lmid, Lhigh)`
- `OctoWeave.compute_levels_bands_by_mean_prob(n, thresholds, levels)`
- `OctoWeave.run_pipeline(..., policy='quantiles'|'bands_mean'|'uniform', policy_args=...)` → runs points→hierarchy→forest→CSV→viz in one call

### Python example
- `python python/examples/run_pipeline_demo.py`
  - Writes PGM/SVG slices under `examples_out/python/`
- `python python/examples/render_csv.py examples_out/leaves.csv 0 -1 examples_out/python/render_slice.pgm examples_out/python/render_hist.svg`
  - Renders directly from a CSV using the Python wrapper
- `python python/examples/parquet_pipeline_demo.py`
  - Generates a sample Parquet (if missing), builds a hierarchy from it, and renders a slice

### Python CLI
After building the shared library (or installing the wheel), you can run the command-line pipeline:
```bash
python -m octoweave_py.cli --points-csv examples_out/leaves.csv --columns x,y,z \
  --n 2 --outdir examples_out/python --policy quantiles --q-lo 0.2 --q-hi 0.8 --Llow 4 --Lmid 7 --Lhigh 10
```
Or install the wheel and use the entry point:
```bash
pip install dist/octoweave_py-*.whl
octoweave-pipeline --parquet examples_out/python/points.parquet --columns x,y,z --n 2 --outdir examples_out/python
```

Optional Python deps:
- `numpy` to pass a `(k,3)` array of points directly.
- `pandas` (+ `pyarrow` or `fastparquet`) to load Parquet files.

## Visualization CLI
`octoweave_viz --csv leaves.csv --slice_z 0 --depth 3 --out slice.pgm --hist hist.svg`

## Notes
- When OctoMap/p4est are disabled, stubs are used and tests/examples still run.
- Prefer RPATH over DYLD_LIBRARY_PATH/LD_LIBRARY_PATH where possible.
