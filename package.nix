{
  capnproto
, ekam
, notcurses
, gtest
, stdenv
, which
, debug ? true
}:

let
  name = "notcurses-capnp";

  create-ekam-rules-link = ''
    ln --symbolic --force --target-directory=src \
      "${ekam.src}/src/ekam/rules"

    mkdir --parents src/compiler
    ln --symbolic --force "${capnproto}/bin/capnp" src/compiler/capnp
    ln --symbolic --force "${capnproto}/bin/capnpc-c++" src/compiler/capnpc-c++

    mkdir --parents src/capnp/compat
    ln --symbolic --force "${capnproto.src}/c++/src/capnp/compat/byte-stream.capnp" src/capnp/compat/
    ln --symbolic --force "${capnproto.src}/c++/src/capnp/compat/byte-stream.h" src/capnp/compat/
    ln --symbolic --force "${capnproto.src}/c++/src/capnp/compat/byte-stream.c++" src/capnp/compat/
#    ln --symbolic --force "${capnproto.src}/c++/src/capnp/compat/http-over-capnp.capnp" src/capnp/compat/
#    ln --symbolic --force "${capnproto.src}/c++/src/capnp/compat/http-over-capnp.h" src/capnp/compat/
#    ln --symbolic --force "${capnproto.src}/c++/src/capnp/compat/http-over-capnp.c++" src/capnp/compat/
  '';

in

stdenv.mkDerivation {

  inherit name;
  src = ./.;

  buildInputs = [
    capnproto
    notcurses
  ];

  nativeBuildInputs = [
    ekam
    gtest
    which
  ];

  propagatedBuildInputs = [
  ];

  CAPNPC_FLAGS = [
    "-I${capnproto}/include"
  ];

  shellHook = create-ekam-rules-link;

  buildPhase = ''
    ${create-ekam-rules-link}
    make ${if debug then "debug" else "release"}
  '';

  installPhase = ''
    install --verbose -D --mode=644 \
      --target-directory="''${!outputLib}/lib" \
      lib${name}.a

    install --verbose -D --mode=644 \
      --target-directory="''${!outputInclude}/include/${name}" \
      src/*.capnp \
      src/*.capnp.h \
      tmp/*.capnp.h \
      src/*.h 
  '';
}
