# OctoWeave — Nix Flake Dev Environment

This repo includes a Nix flake that provisions a C++ toolchain, CMake/Ninja, OctoMap, p4est (C), OpenMPI, and a project Python. Two dev shells are provided:

- `default`: Pure Nix Python (`python.withPackages`) and libraries on `PATH`/`CMAKE_PREFIX_PATH`.
- `staged`: Adds a project-local `.venv` and symlinks headers/libs into `./.deps` for easy inspection or build systems that expect in-tree deps.

## Prereqs

- Install Nix (multi-user) and enable flakes:
  - macOS: `sh <(curl -L https://nixos.org/nix/install)`
  - Add to `~/.config/nix/nix.conf`:
    ```
    experimental-features = nix-command flakes
    ```
- Optional (recommended): `direnv` + `nix-direnv` for auto-activation.

## Quick start

1) Enter the pure nix dev shell:

```bash
nix develop     # or: nix develop .
```

You’ll get `clang` (macOS) or `gcc`, `cmake`, `ninja`, `pkg-config`, `octomap`, `p4est`, `openmpi`, and a Python with common packages.

2) Or enter the staged shell (creates `.venv` and `./.deps`):

```bash
nix develop .#staged
```

- Python venv is created at `./.venv` and populated from `requirements.txt` if present.
- Headers/libs for OctoMap and p4est are symlinked under `./.deps/{include,lib}`.

## Building with CMake

From either shell:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DOCTOWEAVE_WITH_OCTOMAP=ON -DOCTOWEAVE_WITH_P4EST=ON
cmake --build build -j
ctest --test-dir build -j
```

Nix sets up proper `CMAKE_PREFIX_PATH`/`PKG_CONFIG_PATH` so `find_package(OctoMap)` and `pkg-config p4est` should work out of the box.

## Verifying deps inside the shell

```bash
pkg-config --modversion octomap || echo "octomap not found"
pkg-config --modversion p4est   || echo "p4est not found"
```

## Notes on macOS

- The shell uses LLVM `clang` on Darwin to match nixpkgs defaults. If you prefer Apple clang, adjust `flake.nix`.
- SIP blocks some `DYLD_*` propagation from non-login shells; using `nix develop` from a Terminal app works as expected.

## Switching off Homebrew/MacPorts in this repo

- You can keep Homebrew/MacPorts installed system-wide; while inside `nix develop`, nix-provided tools are first on `PATH`.
- If you used `brew`-installed MPI/Boost/Eigen before, do not pass their paths to CMake; let Nix-provided ones be discovered.

## Customizing Python

- Pure nix route: edit `pyEnv` in `flake.nix` (search for `withPackages`) to add/remove Python packages.
- Local venv route: add packages to `requirements.txt`. The `staged` shell installs them into `.venv`.

## Troubleshooting

- If `p4est`/`octomap` are missing for your platform in the pinned `nixos-24.05`, switch to `nixpkgs-unstable` in `flake.nix` and run `nix flake update`.
- If a build system insists on in-tree paths, use the `staged` shell and point CMake to `./.deps` if needed.

