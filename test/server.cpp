/*
 */
//#include "TcpRpcChannel.hpp"
//#include "RpcController.hpp"
//#include "TcpRpcServer.hpp"
//#include "UdpRpcChannel.hpp"
#include "UdpRpcServer.hpp"
#include "EchoTestServer.hpp"
//#include "EchoTestClient.hpp"
#include <boost/shared_ptr.hpp>
#include <iostream>

/*
 *
 */
int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cerr << "Missing port argument" << std::endl;
    return 1;
  }
  
  std::cout << "server port : " << argv[1] << std::endl;
  shared_ptr<pbrpcpp::UdpRpcServer> rpcServer( new pbrpcpp::UdpRpcServer( "localhost", argv[1] ) );
  shared_ptr< EchoTestServer<pbrpcpp::UdpRpcServer> > testServer( new EchoTestServer<pbrpcpp::UdpRpcServer>( rpcServer, 0 ) );
  
  testServer->start();
  
//  pbrpcpp::RpcController controller;
//  
//  string addr, port;
//  while( ! rpcServer->getLocalEndpoint( addr, port ) );
//  
//  shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
//  EchoTestClient client( channel );
//  echo::EchoRequest request;
//  echo::EchoResponse response;
//  
//  request.set_message("hello, world");
//  client.echo( &controller, &request,&response, NULL, 5000 );
//  
//  EXPECT_FALSE( controller.Failed() );
//  EXPECT_EQ( response.response(), "hello, world");
  int temp;
  std::cin >> temp;
  
  testServer->stop();
  
  return 0;
}

