{
  description = "KeysMultitude C++20 library and LLVM 18 development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    generalFunctions = {
      url = "git+https://github.com/DREC-Drillers/generalFunctions.git?ref=binary_trees_rework";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, generalFunctions, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages_18;
      gtest = pkgs.gtest.override { stdenv = llvm.stdenv; };
      python = pkgs.python3.withPackages (packages: [ packages.pyyaml packages.matplotlib ]);
      generalFunctionsPackage = generalFunctions.packages.${system}.default.override {
        buildLexer = false;
        buildGeometry = false;
        buildTree = true;
        buildBvh = false;
      };
      mkShell = pkgs.mkShell.override { stdenv = llvm.stdenv; };
      package = llvm.stdenv.mkDerivation {
        pname = "keys-multitude";
        version = "0.1.0";
        src = self;
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
        propagatedBuildInputs = [ generalFunctionsPackage ];
        buildInputs = [ gtest ];
        cmakeFlags = [
          "-DKEYS_MULTITUDE_BUILD_EXECUTABLE=OFF"
          "-DKEYS_MULTITUDE_BUILD_TESTS=ON"
          "-DKEYS_MULTITUDE_INSTALL=ON"
        ];
        doCheck = true;
        checkPhase = ''
          runHook preCheck
          ctest --output-on-failure --no-tests=error
          runHook postCheck
        '';
      };
      installedConsumerCheck = pkgs.runCommand "keys-multitude-installed-consumer" {
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja llvm.stdenv.cc ];
        buildInputs = [ package ];
      } ''
        cmake -S ${self}/tests/consumer -B build -G Ninja \
          -DCMAKE_CXX_COMPILER=${llvm.stdenv.cc}/bin/clang++
        cmake --build build
        ctest --test-dir build --output-on-failure --no-tests=error
        touch "$out"
      '';
      qualityTools = map (tool: pkgs.writeShellScriptBin "${tool}-18" ''
        exec ${llvm.clang-tools}/bin/${tool} "$@"
      '') [ "clang-format" "clang-tidy" ];
    in
    {
      packages.${system}.default = package;
      checks.${system} = {
        default = package;
        installed-consumer = installedConsumerCheck;
      };
      devShells.${system}.default = mkShell {
        buildInputs = [ generalFunctionsPackage gtest pkgs.gbenchmark ];
        packages = [ pkgs.cmake pkgs.ninja pkgs.git python llvm.clang-tools ] ++ qualityTools;
        shellHook = ''
          echo "KeysMultitude environment: C++20, LLVM 18, CMake, Ninja and Python."
        '';
      };
    };
}
