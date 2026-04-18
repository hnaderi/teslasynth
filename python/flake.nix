{
  description = "teslasynth Python package dev environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    uv2nix = {
      url = "github:pyproject-nix/uv2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    pyproject-nix = {
      url = "github:pyproject-nix/pyproject.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        python = pkgs.python3;
      in
      {
        devShells.default = pkgs.mkShell {
          packages = [
            # uv manages the .venv and Python deps
            pkgs.uv

            # Native build tools for the C++ extension
            pkgs.cmake
            pkgs.ninja
            pkgs.gcc

            # Python interpreter (uv will use this via UV_PYTHON)
            python

            # Build backend + nanobind must be importable during the C++ build.
            # With --no-build-isolation, uv skips the isolated build env and
            # uses whatever is already in the shell instead.
            python.pkgs.scikit-build-core
            python.pkgs.nanobind
            python.pkgs.setuptools-scm
          ];

          env = {
            # Tell uv which Python to use so the .venv matches the nix one.
            UV_PYTHON = python.interpreter;
            UV_NO_BUILD_ISOLATION = "1";
          };

          shellHook = ''
            echo "teslasynth dev environment"
            uv sync --group dev 
            . .venv/bin/activate
            export PATH="${pkgs.ruff}/bin:$PATH"
          '';

        };
      }
    );
}
