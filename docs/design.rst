Design Decisions
================

This page summarizes the main engineering choices in OctoWeave and the rationale
behind them.

Emission Resolution vs. Depth
-----------------------------

We prioritize an explicit emission resolution rather than directly exposing a target
depth. The OctoMap "tree depth" depends on the input resolution (``res``) and the
internal tree structure; tying user intent to a specific depth makes configuration
brittle. Using ``emit_res`` lets you say "emit at 10 cm" and we compute the nearest
coarser level, with a "max_depth_cap" safety limit to avoid runaway refinement.

Threshold τ and Child‑Evidence Guard
------------------------------------

We define leaf vs. internal nodes using a probability threshold ``τ`` (optional
log‑odds comparison), plus a child‑evidence guard: a parent can only refine if there
is at least one observed child at the next level. This avoids creating internal nodes
that have no supporting observations and keeps the hierarchy compact and meaningful.

Bottom‑Up Probability Union (Stability)
---------------------------------------

Rollups use a numerically stable union: ``P = 1 - Π (1 - p_i)``. Missing children are
treated as ``p_unknown``. This balances precision and robustness, avoids NaNs, and is
consistent with OctoMap conventions. Rollup order is deterministic.

Hierarchy Keys and Determinism
------------------------------

We represent nodes via ``NDKey { Key3 k; uint16_t d; }`` with custom hash. All merges
and traversals use deterministic iteration (e.g., predictable child ordering), so
results are reproducible across runs and thread counts (given fixed inputs).

p8est Mapping and Policies
--------------------------

We map global keys into (tree, local) under a brick layout and split by modulo/divide.
Per‑tree target levels are chosen by a policy function. We provide uniform, linear by
leaf count, quantile bands, and mean‑probability bands. Refine uniformly per tree, then
balance. This yields a compact AMR that reflects occupancy evidence while preserving
determinism and downstream Simulator/HPC friendliness.

Per‑Quadrant Data: Mean Probabilities
-------------------------------------

We initialize each quadrant’s user data with the mean of contributing hierarchy leaf
probabilities (at the tree’s target level). This is a good default for visualization
and pre‑simulation staging; more advanced aggregations (e.g., occupancy variance,
log‑odds, counts) can be added later.

Python Wrapper via ctypes
-------------------------

We expose a minimal C API and use ``ctypes`` for Python rather than pybind11:

* No C++ runtime ABI concerns; easy wheel builds via scikit‑build‑core.
* Decoupled from Python’s extension API; simpler cross‑platform builds.

Visualization Choices
---------------------

For low‑level checks, we emit PGM/PPM and SVG histograms (CLI). For richer displays,
we use matplotlib (Python) with PNG outputs, discrete occupancy presets, projections,
montages, and overlays. The Agg backend supports headless environments. We also export
slice grids (CSV) to aid validation and testing.

