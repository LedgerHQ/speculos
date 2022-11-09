with import <nixpkgs> {};
let
  crossSystem = pkgsCross.armv7l-hf-multiplatform;
  crossOpenssl = (crossSystem.pkgs.openssl.override {
    static = true;
  });
  gccWrapper = writeShellScriptBin "arm-linux-gnueabihf-gcc" ''
    exec ${crossSystem.stdenv.cc}/bin/armv7l-unknown-linux-gnueabihf-gcc "$@"
  '';
in
crossSystem.stdenv.mkDerivation {
  name = "speculos";
  nativeBuildInputs = [ cmake gccWrapper ];
  buildInputs = [
    crossOpenssl
    crossSystem.pkgs.glibc.static
  ];

  src = ./.;
  cmakeFlags = "-D PRECOMPILED_DEPENDENCIES_DIR=${lib.getLib crossOpenssl}/lib";

  installPhase = ''
    mkdir -p $out/
    cp ./src/launcher $out/
  '';
}
