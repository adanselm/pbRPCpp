dofile("options.lua")

-------------------------------------------------------------------------------
-- Main Solution
-------------------------------------------------------------------------------
solution "PbRPCpp"
  configurations { "Debug", "Release" }
  location    (_ACTION )
  objdir      ( "../obj/" .. _ACTION )
  platforms   { "x32", "x64", "universal" }
  language "C++"

  -----------------------------------------------------------------------------
  -- Generic Setup
  -----------------------------------------------------------------------------
  configuration {"x32", "Debug"}
    targetdir ( "../bin/" .. _ACTION .. "/x32/Debug" )
    defines { "_DEBUG", "DEBUG" }
    flags { "Symbols"}

  configuration {"x64", "Debug"}
    targetdir ( "../bin/" .. _ACTION .. "/x64/Debug" )
    defines { "_DEBUG", "DEBUG" }
    flags { "Symbols"}

  configuration {"x32","Release"}
    targetdir ( "../bin/" .. _ACTION .. "/x32/Release" )
    defines { "NDEBUG" }
    flags { "OptimizeSpeed"} 
    flags { "EnableSSE2"}

  configuration {"x64","Release"}
    targetdir ( "../bin/" .. _ACTION .. "/x64/Release" ) 
    defines { "NDEBUG" }
    flags { "OptimizeSpeed"} 
    flags { "EnableSSE2"}

  configuration {"universal","Debug"}
    targetdir ( "../bin/" .. _ACTION .. "/universal/Debug" )
    defines { "_DEBUG", "DEBUG" }
    flags { "Symbols"}

  configuration {"universal","Release"}
    targetdir ( "../bin/" .. _ACTION .. "/universal/Release" ) 
    defines { "NDEBUG" }
    flags { "OptimizeSpeed"} 
    flags { "EnableSSE2"}

-------------------------------------------------------------------------------
-- Projects
-------------------------------------------------------------------------------
dofile("libpbrpcpp.lua")
dofile("pbrpcpptests.lua")

-------------------------------------------------------------------------------
-- Dependencies
-------------------------------------------------------------------------------
--dofile("protobuf.lua")
--dofile("gtest.lua")

if _ACTION == "clean" then
  for action in premake.action.each() do
    os.rmdir(action.trigger)
    os.rmdir("../obj/" .. action.trigger)
    os.rmdir("../bin/" .. action.trigger)
  end
end


