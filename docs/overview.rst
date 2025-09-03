Overview
========

OctoWeave is a C++17 pipeline for building chunked occupancy maps and mapping them into a p4est forest, with optional Python bindings and visualization tools.

Goals
-----

- Partition points into an n×n×n chunk grid
- Build per–chunk OctoMap trees (optional real OctoMap integration)
- Bottom–up union into a global hierarchy of nodes
- Map hierarchy into a p4est forest with n³ root trees (optional real p4est integration)
- Provide CLI and Python helpers to visualize and automate workflows
- Strong unit and integration tests for each phase

Key Components
--------------

- ``octoweave::ChunkGrid``: spatial tiling and mapping utilities
- ``octoweave::OctoChunker``: per–chunk tree build and export (real OctoMap when enabled)
- ``octoweave::make_hierarchy_from_workers``: bottom–up union to a hierarchy
- ``octoweave::P4estBuilder``: build/prepare p4est forest with per–quadrant data
- ``octoweave_viz``: CLI to render 2D slices and depth histograms from CSV
- Python wrapper (ctypes): high–level pipeline helpers and CLI

Build Flavors
-------------

- Stub–only: No external deps (default). All APIs work with in–memory stubs.
- OctoMap: Enable with ``-DOCTOWEAVE_WITH_OCTOMAP=ON`` (Homebrew path hints).
- p4est: Enable with ``-DOCTOWEAVE_WITH_P4EST=ON`` (MacPorts path hints).

Motivation & Problem Statement
------------------------------

Most mapping stacks balance fidelity, throughput, and downstream mesh/analysis:

* High‑fidelity occupancy maps (e.g., OctoMap) are ideal for probabilistic fusion,
  but can become heavy at deep resolutions and awkward to distribute.
* Distributed/AMR meshes (e.g., p4est) excel at parallel numerics and adaptive
  refinement, but do not encode Bayesian fusion or sensor semantics by themselves.

OctoWeave bridges these worlds by:

* Ingesting point clouds into per‑chunk OctoMaps at a chosen resolution with a
  configurable sensor model (hit/miss/clamp, free‑space ray carving).
* Performing bottom‑up probabilistic rollups to a compact hierarchy with thresholded
  leaf vs. internal nodes.
* Emitting a p4est forest with n³ root trees (brick layout) and per‑quadrant data
  (e.g., mean occupancy), using deterministic refinement policies.

The result is a fast, testable pipeline that produces an AMR structure ready for
large‑scale simulation, analysis, or meshing — with a principled probabilistic base.

When to Use OctoWeave
---------------------

* Robotics & autonomy: fuse scans into a consistent occupancy hierarchy; export to
  an AMR forest for trajectory planning or simulation.
* Environmental mapping: dense centers and sparse/empty perimeters — refine where it
  matters, keep coarser elsewhere.
* HPC pre‑processing: convert raw occupancy evidence into p4est for PDE/CFD/heat‑map
  solvers that expect adaptive trees and per‑cell data.
* Research prototyping: clear interfaces, strong tests, optional deps (OctoMap/p4est)
  that can be swapped in when available.

Why OctoMap + p4est
-------------------

* OctoMap: mature Bayesian octree occupancy; sensor‑model friendly, robust log‑odds,
  freespace carving via rays.
* p4est: scalable adaptive octree forests for HPC; refinement, balancing, callbacks,
  and per‑quadrant user data.

OctoWeave fuses evidence where it’s natural (OctoMap), then exports to a data
structure built for parallel mesh/AMR workflows (p4est).

High‑Level Workflow
-------------------

1. Chunking: partition world AABB into n×n×n chunks; route points by tile.
2. Per‑chunk build: insert into OctoMaps (or stub), export leaf probabilities at a
   target emission resolution (with a safety depth cap).
3. Hierarchy: bottom‑up union; threshold with a child‑evidence guard; emit leaves vs.
   internals deterministically.
4. p4est mapping: brick partition; split global keys into (tree, local); refine
   uniformly per tree to a policy‑driven target level; initialize per‑quadrant data
   (e.g., mean probability) and balance.
5. Visualization: render slices, projections, montages, and overlays from CSV.

Use‑Case Examples
-----------------

* Dense center, sparse periphery: see ``python/examples/mvn_viz_demo.py`` — samples a
  mixture of multivariate normals and rejects points near the periphery to produce
  varied occupancy. Demonstrates occupancy band presets (free/unknown/occupied),
  projections, montages, and overlays.
* Multi‑chunk fusion: feed each chunk a point cloud, export ``WorkerOut`` per chunk,
  unify into a global hierarchy — leaves carry fused probabilities; internals indicate
  regions where children exist and exceed τ.
* AMR pre‑mesh: choose a per‑tree refinement policy (uniform, quantiles, bands by
  counts/means) to generate a compact, simulation‑ready p4est forest.
