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

        # Project Python (pure nix variant)
        py = pkgs.python312;
        pyEnv = py.withPackages (ps: with ps; [
          pip
          setuptools
          wheel
          numpy
          pandas
          matplotlib
          pillow
          pytest
        ]);
      in
      {
        # Pure nix dev shell (Python provided via withPackages)
        devShells.default = pkgs.mkShell {
          name = "octoweave-dev";
          packages = toolchain ++ cmakeTools ++ coreLibs ++ [ octomap p4est mpi pyEnv git ];

          env = {
            CC = if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang" else "${pkgs.gcc13}/bin/gcc";
            CXX = if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang++" else "${pkgs.gcc13}/bin/g++";
            CMAKE_GENERATOR = "Ninja";
          };

          shellHook = ''
            echo "[octoweave] Dev shell ready: CC=$CC CXX=$CXX"
            echo "[octoweave] OctoMap → $(pkg-config --modversion octomap 2>/dev/null || echo not-found)"
            echo "[octoweave] p4est   → $(pkg-config --modversion p4est 2>/dev/null || echo not-found)"
          '';
        };

        # Dev shell that sets up a local .venv and stages headers/libs under ./.deps
        devShells.staged = pkgs.mkShell {
          name = "octoweave-dev-staged";
          packages = toolchain ++ cmakeTools ++ coreLibs ++ [ octomap p4est mpi pkgs.python312 pkgs.python312Packages.virtualenv git pkgs.coreutils ];

          env = {
            CC = if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang" else "${pkgs.gcc13}/bin/gcc";
            CXX = if pkgs.stdenv.isDarwin then "${pkgs.llvmPackages_17.clang}/bin/clang++" else "${pkgs.gcc13}/bin/g++";
            CMAKE_GENERATOR = "Ninja";
            VENV_DIR = ".venv";
            DEPS_DIR = ".deps";
          };

          shellHook = ''
            set -euo pipefail

            # 1) Project-local Python venv
            if [ ! -d "$VENV_DIR" ]; then
              echo "[octoweave] Creating local Python venv in $VENV_DIR"
              python -m venv "$VENV_DIR"
            fi
            # shellcheck disable=SC1090
            source "$VENV_DIR/bin/activate"
            python -m pip --disable-pip-version-check install --upgrade pip >/dev/null
            if [ -f requirements.txt ]; then
              echo "[octoweave] Installing requirements.txt into $VENV_DIR"
              pip install -r requirements.txt
            fi

            # 2) Stage headers/libs into ./.deps as symlinks for easy inspection/use
            mkdir -p "$DEPS_DIR"/include "$DEPS_DIR"/lib
            # Symlink entire include and lib trees for octomap and p4est
            ln -snf ${octomap}/include "$DEPS_DIR/include/octomap"
            ln -snf ${octomap}/lib     "$DEPS_DIR/lib/octomap"
            ln -snf ${p4est}/include   "$DEPS_DIR/include/p4est"
            ln -snf ${p4est}/lib       "$DEPS_DIR/lib/p4est"

            echo "[octoweave] Staged deps under $DEPS_DIR (symlinks to nix store)"
            echo "[octoweave] Dev shell ready: CC=$CC CXX=$CXX"
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

