{
  pkgs ? import <nixpkgs> {}
, debug ? true
}:

{

  notcurses-capnp = pkgs.callPackage ./package.nix {};
}
