pbRPCpp
=======

pbRPCpp is a small C++ RPC library built on top of boost::asio and google's
protocol buffer, sponsored by [Springbeats](http://springbeats.com), and
released under Boost License.

Goals
-----

PbRPCpp aims to provide a simple Remote Procedure Call API for C++ programs.
It was initially built to simplify client/server communications in desktop
applications that were already using protobuf as a payload format.

Because it is built on top of Boost, it is quite portable.

Getting Started
---------------

### Dependencies

* [Boost](http://www.boost.org/)
* [Google Protobuf](https://code.google.com/p/protobuf/downloads/list)
* [Google Gtest](https://code.google.com/p/googletest/) which is already included in protobuf's archive.

Note that the license for all these components allow a commercial usage (please
have a look at them for details).

### Building

Build system relies on [Premake4](http://industriousone.com/premake/download),
which generates project files for you, depending on the **target** you choose
(gmake, xcode3, xcode4, vs2008, etc. Type `premake4 --help` to see all the
available targets).

For example using gmake:

    cd build
    premake4 --gtestdir=../../protobuf/gtest --boostdir=../../boost --protobufdir=../../protobuf gmake
    make config=debug64

This will build the static lib and unit-tests in the bin/ directory.

### Defining your service in protobuf

    // File: echo.proto
    package echo;
    option cc_generic_services = true;

    message EchoRequest
    {
      required string message = 1;
    }

    message EchoResponse
    {
      required string response = 1;
    }

    service EchoService
    {
      rpc Echo(EchoRequest) returns (EchoResponse);
    }

Compilation with `protoc --cpp_out=. echo.proto` generates echo.pb.h/echo.pb.cc files.

### Using your service in Server code

TODO

### Using your service in Client code

TODO
