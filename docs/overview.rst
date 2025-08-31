Overview
========

OctoWeave is a C++17 pipeline for building chunked occupancy maps and mapping them into a p8est forest, with optional Python bindings and visualization tools.

Goals
-----

- Partition points into an n×n×n chunk grid
- Build per–chunk OctoMap trees (optional real OctoMap integration)
- Bottom–up union into a global hierarchy of nodes
- Map hierarchy into a p8est forest with n³ root trees (optional real p8est integration)
- Provide CLI and Python helpers to visualize and automate workflows
- Strong unit and integration tests for each phase

Key Components
--------------

- ``octoweave::ChunkGrid``: spatial tiling and mapping utilities
- ``octoweave::OctoChunker``: per–chunk tree build and export (real OctoMap when enabled)
- ``octoweave::make_hierarchy_from_workers``: bottom–up union to a hierarchy
- ``octoweave::P8estBuilder``: build/prepare p8est forest with per–quadrant data
- ``octoweave_viz``: CLI to render 2D slices and depth histograms from CSV
- Python wrapper (ctypes): high–level pipeline helpers and CLI

Build Flavors
-------------

- Stub–only: No external deps (default). All APIs work with in–memory stubs.
- OctoMap: Enable with ``-DOCTOWEAVE_WITH_OCTOMAP=ON`` (Homebrew path hints).
- p8est: Enable with ``-DOCTOWEAVE_WITH_P8EST=ON`` (MacPorts path hints).

