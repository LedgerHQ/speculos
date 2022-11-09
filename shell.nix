let
  pkgs = import <nixpkgs> {};
  pythonWithPkgs = pkgs.python3.withPackages (pp: with pp; [
    pyqt5
    construct
    jsonschema
    flask-restful
    pillow
    requests
    mnemonic
    pyelftools
    setuptools            # for pkg_resources
  ]);
  qemuWrapper = pkgs.writeShellScriptBin "qemu-arm-static" ''
    exec ${pkgs.qemu}/bin/qemu-arm "$@"
  '';
  speculos = import ./speculos.nix;
in pkgs.mkShell {
  buildInputs = [
    pythonWithPkgs
    qemuWrapper
  ];

  shellHook = ''
    mkdir -p ./speculos/resources/
    cp ${speculos}/launcher ./speculos/resources/
  '';

  QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.qt5.qtbase.bin}/lib/qt-${pkgs.qt5.qtbase.version}/plugins";
}
