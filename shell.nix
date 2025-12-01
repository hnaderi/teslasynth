let pkgs = import <nixpkgs> { };
in pkgs.mkShell {
  name = "Teslasynth";
  buildInputs = with pkgs; [
    platformio
    ruff
    clang-tools
    tio
    rosegarden
    alsa-utils
  ];
}
