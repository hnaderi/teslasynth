let pkgs = import <nixpkgs> { };
in pkgs.mkShell {
  name = "Teslasynth";
  buildInputs = with pkgs; [
    platformio
    clang-tools
    tio
    rosegarden
    alsa-utils
  ];
}
