# AGENTS.md — OctoWeave

Use this document as the charter for autonomous implementation with Codex CLI.

## Mission
Implement a fast, testable pipeline:
1) Partition points into n×n×n chunks
2) Build per-chunk OctoMap trees (parallelizable)
3) Bottom-up union of children into a global hierarchy
4) Map to a p8est forest with n³ root trees
5) Provide a visualization tool for OctoMap levels
6) Add strong unit/integration tests at each phase

## Constraints
- C++17, CMake. Keep public APIs stable.
- No external deps besides Catch2 (via FetchContent).
- OctoMap and p8est integration code should be behind `OCTOWEAVE_WITH_*` flags and can be developed later. Provide clean stubs + interfaces now.

## Phases (execute strictly in order; gate each phase with tests)
1. **Scaffold sanity**
   - Build `octoweave` and run trivial tests.
   - Ensure CI matrix (at least local ctest) runs.

2. **Chunking**
   - Implement `octoweave::ChunkGrid`: world AABB, n, index mapping, chunk AABB, linear index.
   - Tests: mapping round-trip; boundary clamping; tile coverage.

3. **Probability union**
   - Implement `union_prob8_stable` and helpers.
   - Tests: identities (all zeros → 0; all ones → 1), symmetry, numerical stability with extreme p.

4. **Hierarchy construction**
   - Implement bottom-up rollups from level td → base_depth, and node emission (leaf vs internal) using threshold τ (probability) and child-evidence guard.
   - Tests: small handcrafted pyramids; missing-children fill with p_unknown.

5. **Octo interface**
   - Define IOctoTree + OctoChunker stub (no OctoMap dep yet).
   - Implement in-memory leaf export keyed by `(x,y,z,depth,prob)`.
   - Tests: chunk → leaves at forced `td`.

6. **p8est mapping (stub)**
   - Define `P8estBuilder` API: brick(n³), refine_cb contract, init_cb contract.
   - Provide data structures for “want sets” (no real p8est yet).
   - Tests: mapping from global `(k,d)` to tree-local coords is correct.

7. **Visualization tool**
   - Implement `octoweave_viz` CLI that accepts CSV of `(x,y,z,depth,prob)` and renders:
     - a 2D slice (z=k) PGM/PPM
     - an overview SVG of level histograms
   - Tests: golden-image (or checksum) for tiny CSV.

8. **Parallel chunk execution**
   - Provide a simple thread-pool or per-chunk thread launcher.
   - Tests: determinism given fixed seeds; race-free.

9. **Integrations (optional)**
   - `OCTOWEAVE_WITH_OCTOMAP`: wire OctoMap-based builder.
   - `OCTOWEAVE_WITH_P8EST`: scaffold real p8est build (behind flag).

10. **End-to-end demo**
    - Small synthetic input → chunks → hierarchy → (stubbed) forest → viz.
    - Tests: verifies counts and basic invariants.

## Coding standards
- Namespaces: `octoweave::*`
- Headers under `include/octoweave/…`
- Zero allocations in hot paths where possible; prefer `std::vector` with reserve.
- Document pre/post conditions in headers.

## CI / Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo`
- Build:     `cmake --build build -j`
- Test:      `ctest --test-dir build -j`
