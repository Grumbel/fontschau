{
  description = "Fontschau – a simple Qt6 font viewer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        versionBase = pkgs.lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
        gitRev = self.shortRev or self.dirtyShortRev or "dirty";
        version = if pkgs.lib.strings.hasInfix "-dev" versionBase then
          "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
        else
          versionBase;
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "fontschau";
          inherit version;

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
            qt6.wrapQtAppsHook
          ];

          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtsvg
            freetype
            fontconfig
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DPROJECT_VERSION_FULL=${version}"
          ];

          meta = with pkgs.lib; {
            description = "A simple font viewer built with Qt6";
            homepage = "https://github.com/grumbel/fontschau";
            license = licenses.gpl3Plus;
            maintainers = [ ];
            platforms = platforms.linux;
            mainProgram = "fontschau";
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = with pkgs; [
            cmake
            qt6.qtbase
            qt6.qtsvg
            freetype
            fontconfig
            qt6.qttools
            gdb
          ];
        };
      });
}