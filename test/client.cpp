/*
 */
//#include "TcpRpcChannel.hpp"
#include "RpcController.hpp"
//#include "TcpRpcServer.hpp"
#include "UdpRpcChannel.hpp"
//#include "UdpRpcServer.hpp"
//#include "EchoTestServer.hpp"
#include "EchoTestClient.hpp"
#include <boost/shared_ptr.hpp>
#include <iostream>

/*
 *
 */
int main(int argc, char** argv)
{
  if(argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " PORT MSG_SIZE" << std::endl;
    return 1;
  }
  
  pbrpcpp::RpcController controller;
  const std::string addr("localhost");
  const std::string port(argv[1]);
  int msg_size = 0;
  std::istringstream iss(argv[2]);
  iss >> msg_size;
  std::string bigData(msg_size, 'z');
//  const std::string message(argv[2]);
//  std::cout << "targeting " << addr << ":" << port << " with msg: " << message << std::endl;
  
  shared_ptr<pbrpcpp::UdpRpcChannel> channel( new pbrpcpp::UdpRpcChannel( addr, port ) );
  EchoTestClient client( channel );
  echo::EchoRequest request;
  echo::EchoResponse response;
  
  request.set_message(bigData);
  client.echo( &controller, &request,&response, NULL, 1 );
  
  if( controller.Failed() )
  {
    std::cerr << "Failed." << std::endl;
    return 1;
  }
  
  std::cout << "Received: " << response.response().size() << " bytes" << std::endl;
  return 0;
}

