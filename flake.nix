{
  description = "Teslasynth MCU firmware development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev.url = "github:HTunne/nixpkgs-esp-dev";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      nixpkgs-esp-dev,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        esp-idf = nixpkgs-esp-dev.packages.${system}.esp-idf-full;
        add-headers = pkgs.writeShellScriptBin "add-headers" ''
          copywrite headers "$@"
          (cd python && copywrite headers "$@")
        '';
        format-cpp = pkgs.writeShellScriptBin "format-cpp" ''
          git ls-files -- '*.cpp' '*.c' '*.hpp' '*.h' | xargs clang-format -i
        '';
        format-cpp-check = pkgs.writeShellScriptBin "format-cpp-check" ''
          git ls-files -- '*.cpp' '*.c' '*.hpp' '*.h' | xargs clang-format --dry-run --Werror
        '';
        fw-build = pkgs.writeShellScriptBin "fw-build" ''
          : ''${1:?Usage: fw-build <target>  (e.g. fw-build esp32s3)}
          idf.py -B build/$1 -D IDF_TARGET=$1 build
        '';
        fw-flash = pkgs.writeShellScriptBin "fw-flash" ''
          : ''${1:?Usage: fw-flash <target>  (e.g. fw-flash esp32s3)}
          idf.py -B build/$1 flash
        '';
        fw-monitor = pkgs.writeShellScriptBin "fw-monitor" ''
          : ''${1:?Usage: fw-monitor <target>  (e.g. fw-monitor esp32s3)}
          idf.py -B build/$1 monitor
        '';
        fw-menuconfig = pkgs.writeShellScriptBin "fw-menuconfig" ''
          : ''${1:?Usage: fw-menuconfig <target>  (e.g. fw-menuconfig esp32s3)}
          idf.py -B build/$1 menuconfig
        '';
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            esp-idf

            # Native tests and linting (PlatformIO native env)
            platformio

            # Web dashboard
            pnpm

            # Tooling
            ruff
            xxd
            copywrite

            # Serial monitor
            tio

            # MIDI testing
            rosegarden
            alsa-utils

            add-headers
            format-cpp
            format-cpp-check
            fw-build
            fw-flash
            fw-monitor
            fw-menuconfig
          ];
        };
      }
    );
}
