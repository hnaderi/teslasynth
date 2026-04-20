{
  description = "Teslasynth MCU firmware development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
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
        esp-idf = nixpkgs-esp-dev.packages.${system}.esp-idf-full.override {
          rev = "v6.0";
          sha256 = "sha256-YhON/zUFOVTh8UEvujAXsd9IPaaNmSIP+dSZDE5fyqw=";
        };
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
            clang-tools
            ruff
            xxd
            copywrite

            # Serial monitor
            tio

            # MIDI testing
            rosegarden
            alsa-utils
          ];

          shellHook = ''
            source ${esp-idf}/activate
          '';
        };
      }
    );
}
