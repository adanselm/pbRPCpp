project "libpbrpcpp"
  kind "StaticLib"

  includedirs {
    "../include", "../src",
    _OPTIONS["protobufdir"] .. "/src",
    _OPTIONS["boostdir"],
  }
  files {
    "../src/*.cpp", "../src/*.hpp", "../include/*.hpp",
  }

