{
  description = "Set a primary X11 display for hyprland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    hyprland.url = "github:hyprwm/Hyprland";
  };

  outputs =
    {
      self,
      nixpkgs,
      ...
    }@inputs:
    let
      systems = [
        "x86_64-linux"
        "aarch64"
      ];
      eachSystem = nixpkgs.lib.genAttrs systems;
      pkgsFor = nixpkgs.legacyPackages;
    in
    {
      packages = eachSystem (system: {
        default = self.packages.${system}.hyprXPrimary;
        hyprXPrimary =
          let
            inherit (inputs.hyprland.packages.${system}) hyprland;
            inherit (pkgsFor.${system}) gcc15Stdenv;

            name = "hyprXPrimary";
          in
          gcc15Stdenv.mkDerivation {
            inherit name;
            pname = name;

            src = ./.;

            inherit (hyprland) buildInputs;
            nativeBuildInputs = hyprland.nativeBuildInputs ++ [
              hyprland
            ];

            dontUseCmakeConfigure = true;
            dontUseMesonConfigure = true;
            dontUseNinjaBuild = true;
            dontUseNinjaInstall = true;

            installPhase = ''
              runHook preInstall

              mkdir -p "$out/lib"
              cp ./${name}.so "$out/lib/lib${name}.so"

              runHook postInstall
            '';
          };
      });
    };
}
