{
  description = "KeysMultitude C++20 library and LLVM 18 development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages_18;
      gtest = pkgs.gtest.override { stdenv = llvm.stdenv; };
      python = pkgs.python3.withPackages (packages: [ packages.pyyaml ]);
      package = llvm.stdenv.mkDerivation {
        pname = "keys-multitude";
        version = "0.1.0";
        src = self;
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
        buildInputs = [ gtest ];
        cmakeFlags = [
          "-DKEYS_MULTITUDE_BUILD_TESTS=ON"
          "-DKEYS_MULTITUDE_INSTALL=ON"
          "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"
        ];
        doCheck = true;
        checkPhase = ''
          runHook preCheck
          ctest --output-on-failure --no-tests=error
          runHook postCheck
        '';
      };
      qualityTools = map (tool: pkgs.writeShellScriptBin "${tool}-18" ''
        exec ${llvm.clang-tools}/bin/${tool} "$@"
      '') [ "clang-format" "clang-tidy" ];
    in
    {
      packages.${system}.default = package;
      checks.${system}.default = package;
      devShells.${system}.default = (pkgs.mkShell.override { stdenv = llvm.stdenv; }) {
        inputsFrom = [ package ];
        packages = [ pkgs.cmake pkgs.ninja pkgs.git python llvm.clang-tools ] ++ qualityTools;
        shellHook = ''
          echo "KeysMultitude environment: C++20, LLVM 18, CMake, Ninja and Python."
        '';
      };
    };
}
