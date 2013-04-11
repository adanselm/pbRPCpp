newaction {
  trigger     = "doxy",
  description = "Generate doxygen files using Doxyfile",
  execute = function ()
    -- copy files, etc. here
    os.rmdir("../doc/doxygen/html")
    os.execute("doxygen")
  end
}

-- PREFIX
newoption {
  trigger     = "protobufdir",
  description = "Specify where to find protocol buffers"
}

newoption {
  trigger     = "boostdir",
  description = "Specify where to find boost"
}

newoption {
  trigger     = "gtestdir",
  description = "Specify where to find google test framework"
}

-- Has to be AFTER the newoption declarations or they won't appear in help.
if not _ACTION then
  premake.showhelp()
  os.exit(0)
end


_OPTIONS["protobufdir"] = _OPTIONS["protobufdir"] or "../deps/protobuf"
_OPTIONS["boostdir"] = _OPTIONS["boostdir"] or "../../boost_1_51_0"
_OPTIONS["gtestdir"] = _OPTIONS["gtestdir"] or "../deps/gtest"

