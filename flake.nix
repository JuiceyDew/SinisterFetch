{
  description = "A fork of fastfetch with a 2D rotating logo";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages.default = pkgs.fastfetch.unwrapped.overrideAttrs (oldAttrs: {
          pname = "sinisterfetch";
          version = "custom";
          src = self;

          # Disable version check which expects a binary named fastfetch
          doInstallCheck = false;

          # Rename the binary to sinisterfetch after installation
          postInstall = (oldAttrs.postInstall or "") + ''
            mv $out/bin/fastfetch $out/bin/sinisterfetch
            if [ -f $out/bin/flashfetch ]; then
              rm -f $out/bin/flashfetch
            fi
          '';
        });

        packages.sinisterfetch = self.packages.${system}.default;

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
        };
      }
    );
}
