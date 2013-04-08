/* 
 * File:   EchoTest.cpp
 * Author: Steven
 *
 * Created on March 13, 2013, 9:58 AM
 */


#include "echo.pb.h"
#include "TcpRpcChannel.hpp"
#include "RpcController.hpp"
#include "TcpRpcServer.hpp"
#include "UdpRpcChannel.hpp"
#include "UdpRpcServer.hpp"

using google::protobuf::NewCallback;


class EchoServiceImpl: public echo::EchoService {
public:
    virtual void Echo(::google::protobuf::RpcController* controller,
                       const ::echo::EchoRequest* request,
                       ::echo::EchoResponse* response,
                       ::google::protobuf::Closure* done) {
        response->set_response( request->message() );
        ::sleep( 1 );
        done->Run();
    }
};



void startTcpEchoClient(  pbrpcpp::TcpRpcServer& rpcServer ) {
    pbrpcpp::TcpRpcChannel channel( "localhost", "6890" );
    
    GOOGLE_LOG(INFO) << "TcpRpcChannel is created";
    pbrpcpp::RpcController controller;
    
    channel.setRequestTimeout( 100 );
    echo::EchoService::Stub stub(&channel);
    echo::EchoRequest request;
    echo::EchoResponse response;
    request.set_message("hello, world");
    stub.Echo( &controller, &request, &response, NULL );
    //should timeout for this
    if( controller.Failed()) {
        std::cout << "receive error:" << controller.ErrorText() << std::endl;
    } else {
        std::cout << response.response() << std::endl << std::flush;
    }
    
    request.set_message("do you know");
    controller.Reset();
    channel.setRequestTimeout( 2000 );
    stub.Echo( &controller, &request, &response, NULL );
    if( controller.Failed()) {
        std::cout << "receive error:" << controller.ErrorText() << std::endl;
    } else {
        std::cout << response.response() << std::endl << std::flush;
    }
    
    rpcServer.Shutdown();
}

void testTcpRpc(){
    pbrpcpp::TcpRpcServer rpcServer( "localhost", "6890" );
    EchoServiceImpl echoService;
    
    rpcServer.Export( &echoService );    
    
    boost::thread client_thread(  boost::bind( startTcpEchoClient, boost::ref(rpcServer) ) );
    
    rpcServer.Run();
    
    client_thread.join();
}




void startUdpEchoClient( pbrpcpp::UdpRpcServer& rpcServer ) {
    pbrpcpp::UdpRpcChannel channel( "localhost", "6890" );
    pbrpcpp::RpcController controller;
    
    channel.setRequestTimeout( 2000 );
    echo::EchoService::Stub stub(&channel);
    echo::EchoRequest request;
    echo::EchoResponse response;
    request.set_message("hello, world");

    stub.Echo( &controller, &request, &response, NULL );
    if( controller.Failed()) {
        std::cout << "receive error:" << controller.ErrorText() << std::endl;
    } else {
        std::cout << response.response() << std::endl << std::flush;
    }
    
    channel.setRequestTimeout( 100 );
    request.set_message("do you know");
    stub.Echo( &controller, &request, &response, NULL );
    if( controller.Failed()) {
        std::cout << "receive error:" << controller.ErrorText() << std::endl;
    } else {
        std::cout << response.response() << std::endl << std::flush;
    }
    
    rpcServer.Shutdown();
}

void testUdpRpc( ) {
    pbrpcpp::UdpRpcServer rpcServer( "localhost", "6890" );
    EchoServiceImpl echoService;
    
    rpcServer.Export( &echoService );
    
    boost::thread client_thread(  boost::bind( startUdpEchoClient, boost::ref(rpcServer) ) );
    rpcServer.Run();
    client_thread.join();
}

void emptyLogHandler(google::protobuf::LogLevel level, const char* filename, int line,
                        const std::string& message) {
    
}
/*
 * 
 */
int main(int argc, char** argv) {

    google::protobuf::SetLogHandler( &emptyLogHandler );
    testUdpRpc();
    testTcpRpc();
    return 0;
}


