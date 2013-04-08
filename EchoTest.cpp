/* 
 * File:   EchoTest.cpp
 * Author: Steven
 *
 * Created on March 13, 2013, 9:58 AM
 */


#include "TcpRpcChannel.hpp"
#include "RpcController.hpp"
#include "TcpRpcServer.hpp"
#include "UdpRpcChannel.hpp"
#include "UdpRpcServer.hpp"
#include "EchoTestServer.hpp"
#include "EchoTestClient.hpp"
#include "Timer.hpp"
#include "MethodCallIDGenerator.hpp"

void startTcpEchoClient(  shared_ptr<EchoTestServer<pbrpcpp::TcpRpcServer> > testServer ) {
    shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( "localhost", "6891" ) );
    
    GOOGLE_LOG(INFO) << "TcpRpcChannel is created";
    pbrpcpp::RpcController controller;
    
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    client.echo( &controller, &request,&response, NULL, 100 );
    //should timeout for this
    if( controller.Failed()) {
        std::cout << "receive error:" << controller.ErrorText() << std::endl;
    } else {
        std::cout << response.response() << std::endl << std::flush;
    }
    
    request.set_message("do you know");
    controller.Reset();
    client.echo( &controller, &request, &response, NULL, 2000 );
    if( controller.Failed()) {
        std::cout << "receive error:" << controller.ErrorText() << std::endl;
    } else {
        std::cout << response.response() << std::endl << std::flush;
    }
    
    testServer->stop();
}

void testTcpRpc(){
    shared_ptr<pbrpcpp::TcpRpcServer> rpcServer( new pbrpcpp::TcpRpcServer( "localhost", "6891" ) );
    shared_ptr<EchoTestServer<pbrpcpp::TcpRpcServer> > testServer( new EchoTestServer<pbrpcpp::TcpRpcServer>( rpcServer, 1 ) );

    testServer->start();
    startTcpEchoClient( testServer );
}



void startUdpEchoClient( pbrpcpp::UdpRpcServer& rpcServer ) {
    pbrpcpp::UdpRpcChannel channel( "localhost", "6881" );
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
    pbrpcpp::UdpRpcServer rpcServer( "localhost", "6881" );
    EchoServiceImpl echoService(1);
    
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


