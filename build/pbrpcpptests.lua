-----------------------------------------------------------------------------
-- Unit-tests definition
-----------------------------------------------------------------------------
project "Tests"
  kind "ConsoleApp"
  files { "../test/*.hpp",
          "../test/*.h",
          "../test/echo.pb.cc",
          "../test/EchoTestServer.cpp",
          "../test/EchoTestClient.cpp",
          "../test/RpcUnitTest.cpp",
        }
  includedirs { "../test", "../src", "../include",
                 _OPTIONS["gtestdir"] .. "/include/" ,
                 _OPTIONS["boostdir"],
                 _OPTIONS["protobufdir"] .. "/src/" }
  links { "libpbrpcpp",
          "gtest", "protobuf", "boost_system", "boost_thread", "boost_date_time"
        }
  libdirs {_OPTIONS["boostdir"] .. "/stage/lib/",
           _OPTIONS["gtestdir"] .. "/lib/.libs",
           _OPTIONS["protobufdir"] .. "/src/.libs",
          }

  configuration {"not windows"}
    links { "pthread" }

  configuration {"linux"}
    links { "rt" }

