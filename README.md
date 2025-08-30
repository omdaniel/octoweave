# OctoWeave

A scaffold for building a chunked OctoMap → p8est pipeline with:
- n×n×n chunking and per-chunk OctoMap insertion (parallelizable)
- bottom-up union of children (probabilistic)
- export to a p8est forest (n³ roots via brick connectivity)
- a CLI visualization tool for OctoMap levels (stubbed; to be implemented)
- stepwise agent prompts (Codex CLI) to implement & test modules in phases

## Quick start
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build
