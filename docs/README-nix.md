# OctoWeave — Nix Flake Dev Environment

This repo includes a Nix flake that provisions a C++ toolchain, CMake/Ninja, OctoMap, p4est (C), OpenMPI, and Python via `uv`. A single dev shell is provided, which sets up a Python 3.11 virtual environment using `uv` based on `python/pyproject.toml`.

## Prereqs

- Install Nix (multi-user) and enable flakes:
  - macOS: `sh <(curl -L https://nixos.org/nix/install)`
  - Add to `~/.config/nix/nix.conf`:
    ```
    experimental-features = nix-command flakes
    ```
- Optional (recommended): `direnv` + `nix-direnv` for auto-activation.

## Quick start

1) Enter the dev shell:

```bash
nix develop     # or: nix develop .
```

You’ll get `clang` (macOS) or `gcc`, `cmake`, `ninja`, `pkg-config`, `octomap`, `p4est`, `openmpi`, and `uv` + Python 3.11. On entry, the shell runs `uv venv`/`uv sync` inside `python/` to create `python/.venv` from `python/pyproject.toml` (including `dev` and `test` extras). Activate it with `source python/.venv/bin/activate` or use `uv run`.

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

- Edit `python/pyproject.toml` to change dependencies. Then run `uv sync` inside `python/`.
- The project targets Python 3.11+ (`requires-python = ">=3.11"`). Lint config (Ruff) is set to `py311`.

## Troubleshooting

- If `p4est`/`octomap` are missing for your platform in the pinned `nixos-24.05`, switch to `nixpkgs-unstable` in `flake.nix` and run `nix flake update`.
- If `uv sync` fails due to networking or index issues, you can still use the nix-provided Python directly (`${python311}/bin/python3.11`).
