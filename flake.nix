{
  description = "OctoWeave dev environment (nix flake)";

  inputs = {
    # Pick a reasonably recent channel; adjust if you prefer unstable
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };

        # Toolchain
        toolchain = if pkgs.stdenv.isDarwin then [ pkgs.llvmPackages_17.clang pkgs.llvmPackages_17.lld ] else [ pkgs.gcc13 ];
        cmakeTools = with pkgs; [ cmake ninja pkg-config ];

        # Core C++ deps you may want available while building
        coreLibs = with pkgs; [ eigen boost ];

        # Requested libraries (provided by nixpkgs)
        # Note: p4est is a C library; it may pull in its own libsc internally.
        # If your pin lacks p4est/octomap on your platform, consider switching to
        # an `unstable` nixpkgs pin.
        mpi = pkgs.openmpi;
        octomap = pkgs.octomap;
        p4est = pkgs.p4est;
        # Python toolchain: prefer Python 3.11 baseline and `uv` for dependency management
        py = pkgs.python311;
        uv = pkgs.uv;
      in
      {
        # Single dev shell: C++ toolchain + optional libs + Python 3.11 + uv
        devShells.default = pkgs.mkShell {
          name = "octoweave-dev";
          packages = toolchain ++ cmakeTools ++ coreLibs ++ [ octomap p4est mpi py uv git pkgs.coreutils ];

          env = {
            CC = if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang" else "${pkgs.gcc13}/bin/gcc";
            CXX = if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang++" else "${pkgs.gcc13}/bin/g++";
            CMAKE_GENERATOR = "Ninja";
            # Where the pyproject.toml lives and where to place the venv
            PY_PROJ_DIR = "python";
            VENV_DIR = "python/.venv";
          };

          shellHook = ''
            set -euo pipefail
            echo "[octoweave] Dev shell ready: CC=$CC CXX=$CXX"
            echo "[octoweave] OctoMap → $(pkg-config --modversion octomap 2>/dev/null || echo not-found)"
            echo "[octoweave] p4est   → $(pkg-config --modversion p4est 2>/dev/null || echo not-found)"

            # Setup Python via uv using the project's pyproject.toml
            if [ -d "$PY_PROJ_DIR" ] && [ -f "$PY_PROJ_DIR/pyproject.toml" ]; then
              echo "[octoweave] Syncing Python deps with uv (py>=3.11)"
              # Prefer nix-provided Python 3.11
              PYBIN="${py}/bin/python3.11"
              pushd "$PY_PROJ_DIR" >/dev/null
              if [ ! -d .venv ]; then
                uv venv --python "$PYBIN" .venv || true
              fi
              # Activate and sync dev/test extras
              # shellcheck disable=SC1091
              source .venv/bin/activate
              uv sync -E dev -E test || true
              popd >/dev/null
            else
              echo "[octoweave] Skipping uv sync: $PY_PROJ_DIR/pyproject.toml not found"
            fi
          '';
        };

        # Optional default package to build the repo with CMake via `nix build`.
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "octoweave";
          version = "dev";
          src = ./.;
          nativeBuildInputs = cmakeTools;
          buildInputs = coreLibs ++ [ octomap p4est mpi ];
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            # Prefer the nix-provided toolchain
            ("-DCMAKE_C_COMPILER=" + (if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang" else "${pkgs.gcc13}/bin/gcc"))
            ("-DCMAKE_CXX_COMPILER=" + (if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang++" else "${pkgs.gcc13}/bin/g++"))
          ];
        };
      }
    );
}
