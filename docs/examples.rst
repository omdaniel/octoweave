Examples
========

C++
---

- ex01: Build a synthetic hierarchy, write CSV
  ``./build/ex01_synthetic_hierarchy``
- ex02: Forest policies (real when p4est ON)
  ``./build/ex02_forest_policies``
- ex03: Render a CSV slice & histogram
  ``./build/ex03_viz_export``
- ex04: Read CSV → Hierarchy → build forest
  ``./build/ex04_csv_to_forest examples_out/leaves.csv``
- ex05: Parallel per–chunk determinism demo
  ``./build/ex05_parallel_chunks``

Python
------

- Full pipeline demo:
  ``python python/examples/run_pipeline_demo.py``
- Render from CSV:
  ``python python/examples/render_csv.py examples_out/leaves.csv 0 -1``
- Parquet → pipeline → viz:
  ``python python/examples/parquet_pipeline_demo.py``
