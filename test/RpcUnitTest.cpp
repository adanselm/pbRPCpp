/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
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
#include "Util.hpp"
#include "RpcMessage.hpp"
#include "ShmConnection.hpp"
#include "ShmRpcServer.hpp"
#include "ShmRpcChannel.hpp"
#include <gtest/gtest.h>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>


using google::protobuf::NewCallback;
using google::protobuf::MethodDescriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::Message;
using ::testing::InitGoogleTest;
using std::istringstream;
using std::ostringstream;

void handleTimeout( bool& called ) {
    called = true;
}

void responseReceived( boost::condition_variable* cond ) {
    cond->notify_all();
}

void responseReceived( bool* received ) {
    *received = true;
}

TEST( RPCMessageTest, RequestSerialization ) {
    string callId = pbrpcpp::MethodCallIDGenerator::generateID();
  const MethodDescriptor* methodDescriptor( DescriptorPool::generated_pool()->FindMethodByName( "echo.EchoService.Echo") );
    
    EXPECT_TRUE( methodDescriptor != 0 );
    
    echo::EchoRequest request;
    request.set_message("hello, world");
    ostringstream out;
    
    pbrpcpp::RpcMessage::serializeRequest( callId, *methodDescriptor,  request, out );
    
    istringstream in( out.str() );
    EXPECT_EQ( pbrpcpp::Util::readInt( in ), (int)pbrpcpp::RpcMessage::REQUEST_MSG );
    
    string newCallId;
    const MethodDescriptor* newMethodDescriptor = 0;
    shared_ptr<Message> newRequest;
    
    pbrpcpp::RpcMessage::parseRequestFrom( in, newCallId, newMethodDescriptor, newRequest );
    
    EXPECT_EQ( callId, newCallId );
    EXPECT_EQ( methodDescriptor, newMethodDescriptor );
    EXPECT_TRUE( newRequest != 0 );
    EXPECT_TRUE( pbrpcpp::Util::equals( request, *newRequest ) );
}

TEST( RPCMessageTest, ResponseSerialization ) {
    
    string callId = pbrpcpp::MethodCallIDGenerator::generateID();
    pbrpcpp::RpcController controller;
    echo::EchoResponse response;
    response.set_response( "test");
    
    EXPECT_FALSE( controller.IsCanceled());
    EXPECT_FALSE( controller.Failed() );
    ostringstream out;
    
    pbrpcpp::RpcMessage::serializeResponse( callId, controller,  &response, out );    
       
    istringstream in( out.str() );
    string newCallId;
    shared_ptr<Message> newResponse;
    pbrpcpp::RpcController newController;
    
    
    EXPECT_EQ( pbrpcpp::Util::readInt( in ), (int)pbrpcpp::RpcMessage::RESPONSE_MSG );
    
    pbrpcpp::RpcMessage::parseResponseFrom( in, newCallId, newController, newResponse );
        
    EXPECT_EQ( callId, newCallId );
    EXPECT_FALSE( newController.IsCanceled());
    EXPECT_FALSE( newController.Failed() );
    EXPECT_TRUE( newResponse != 0 );
    EXPECT_TRUE( pbrpcpp::Util::equals( response, *newResponse ) );
    
    //failed case test
    controller.SetFailed( "fail to connect to server");
    EXPECT_TRUE( controller.Failed() );
    ostringstream out_2; 
    
    pbrpcpp::RpcMessage::serializeResponse( callId, controller,  0, out_2 );
    
    istringstream in_2( out_2.str() );
    newCallId.clear();
    newController.Reset();
    newResponse.reset();
    
    EXPECT_FALSE( newCallId == callId );
    EXPECT_EQ( pbrpcpp::Util::readInt( in_2 ), (int)pbrpcpp::RpcMessage::RESPONSE_MSG );
    
    pbrpcpp::RpcMessage::parseResponseFrom( in_2, newCallId, newController, newResponse );
        
    EXPECT_EQ( callId, newCallId );
    EXPECT_FALSE( newController.IsCanceled());
    EXPECT_TRUE( newController.Failed() );    
    EXPECT_TRUE( newResponse == 0 );
    
    //reset the controller test
    controller.Reset();
    EXPECT_FALSE( controller.IsCanceled());
    EXPECT_FALSE( controller.Failed() );    
}

TEST( RPCMessageTest, CancelSerialization ) {
    string callId = pbrpcpp::MethodCallIDGenerator::generateID();
    
    ostringstream out;
    pbrpcpp::RpcMessage::serializeCancel( callId, out );
    
    istringstream in( out.str() );
    
    string newCallId;
    
    EXPECT_EQ( pbrpcpp::Util::readInt( in ), (int)pbrpcpp::RpcMessage::CANCEL_MSG );
    pbrpcpp::RpcMessage::parseCancelFrom( in, newCallId );
    
    EXPECT_EQ( callId, newCallId );     
    
}

TEST( RPCMessageTest, NetPacketSerialization ) {
    string msg = "this is a test packet";
    
    ostringstream out;
    string netPacket = pbrpcpp::RpcMessage::serializeNetPacket( msg );
    
    EXPECT_TRUE( netPacket.length() > msg.length() );
    
    string newMsg;
    EXPECT_TRUE( pbrpcpp::RpcMessage::extractNetPacket( netPacket, newMsg ) );
    
    EXPECT_EQ( msg, newMsg );
    EXPECT_EQ( netPacket.length(), 0 );
    newMsg.clear();
    
    EXPECT_FALSE( pbrpcpp::RpcMessage::extractNetPacket( netPacket, newMsg ) );          
}


        
TEST( TimerTest, Timeout ) {
    pbrpcpp::Timer<int> timer;
    bool called = false;
    
    timer.add( 1, 2000, boost::bind( handleTimeout, boost::ref(called) ));
    
    sleep( 1 );
    
    EXPECT_FALSE( called );
    
    sleep( 2 );
    
    EXPECT_TRUE( called );
}

TEST( TimerTest, Cancel ) {
    pbrpcpp::Timer<int> timer;
    bool called = false;
    
    timer.add( 1, 2000, boost::bind( handleTimeout, boost::ref(called) ));
    
    sleep( 1 );
    
    EXPECT_FALSE( called );
    timer.cancel( 1 );
    sleep( 2 );
    
    EXPECT_FALSE( called );
}

TEST( TcpRpcTest, SyncSuccess ) {
    shared_ptr<pbrpcpp::TcpRpcServer> rpcServer( new pbrpcpp::TcpRpcServer( "localhost", "0" ) );
    shared_ptr< EchoTestServer<pbrpcpp::TcpRpcServer> > testServer( new EchoTestServer<pbrpcpp::TcpRpcServer>( rpcServer, 1 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    string addr, port;
    while( ! rpcServer->getLocalEndpoint( addr, port ) );
    
    shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    client.echo( &controller, &request,&response, NULL, 5000 );
    
    EXPECT_FALSE( controller.Failed() );
    EXPECT_EQ( response.response(), "hello, world");
    testServer->stop();
}


TEST( TcpRpcTest, AsyncSuccess ) {
    shared_ptr<pbrpcpp::TcpRpcServer> rpcServer( new pbrpcpp::TcpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::TcpRpcServer> > testServer( new EchoTestServer<pbrpcpp::TcpRpcServer>( rpcServer, 1 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    
    string addr, port;
    while( ! rpcServer->getLocalEndpoint( addr, port ) );
    
    shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
    
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    boost::mutex respMutex;
    boost::condition_variable cond;
    
    client.echo( &controller, &request,&response, NewCallback( responseReceived, &cond ) , 5000 );
    
    {
        boost::unique_lock<boost::mutex> lock(respMutex);
        cond.wait( lock );
    }
    
    EXPECT_TRUE( !controller.Failed() );
    EXPECT_EQ( response.response(), "hello, world");
    testServer->stop();
}

TEST( TcpRpcTest, SyncTimeout ) {
    shared_ptr<pbrpcpp::TcpRpcServer> rpcServer( new pbrpcpp::TcpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::TcpRpcServer> > testServer( new EchoTestServer<pbrpcpp::TcpRpcServer>( rpcServer, 3 ) );
    
    testServer->start();
    
    string addr, port;
    while( ! rpcServer->getLocalEndpoint( addr, port ) );
    
    pbrpcpp::RpcController controller;
    
    shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    client.echo( &controller, &request,&response, NULL, 100 );
    
    EXPECT_TRUE( controller.Failed() );
    testServer->stop();
}

TEST( TcpRpcTest, AsyncTimeout ) {
    shared_ptr<pbrpcpp::TcpRpcServer> rpcServer( new pbrpcpp::TcpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::TcpRpcServer> > testServer( new EchoTestServer<pbrpcpp::TcpRpcServer>( rpcServer, 5 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    string addr, port;
    while( ! rpcServer->getLocalEndpoint( addr, port ) );
    
    shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    boost::mutex respMutex;
    boost::condition_variable cond;
    
    client.echo( &controller, &request,&response, NewCallback( responseReceived, &cond), 100 );
    {
        boost::unique_lock<boost::mutex> lock(respMutex);
        cond.wait( lock );
    }
    EXPECT_TRUE( controller.Failed() );
    testServer->stop();
}

TEST( TcpRpcTest, Cancel ) {
    shared_ptr<pbrpcpp::TcpRpcServer> rpcServer( new pbrpcpp::TcpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::TcpRpcServer> > testServer( new EchoTestServer<pbrpcpp::TcpRpcServer> ( rpcServer, 5 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    string addr, port;
    while( ! rpcServer->getLocalEndpoint( addr, port ) );
    
    shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    boost::mutex respMutex;
    bool received = false;
    
    client.echo( &controller, &request,&response, NewCallback( responseReceived, &received ), 2000 );
    usleep( 100* 1000 );
    controller.StartCancel();
    
    while( !received ) {
        usleep( 100*1000);
    }
    EXPECT_TRUE( controller.IsCanceled() );
    EXPECT_FALSE( controller.Failed() );

    testServer->stop();
}

TEST( UdpRpcTest, SyncSuccess ) {
    shared_ptr<pbrpcpp::UdpRpcServer> rpcServer( new pbrpcpp::UdpRpcServer( "localhost", "0" ) );
    shared_ptr< EchoTestServer<pbrpcpp::UdpRpcServer> > testServer( new EchoTestServer<pbrpcpp::UdpRpcServer>( rpcServer, 1 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    string addr, port;
    
    while( ! rpcServer->getLocalEndpoint(addr, port ));
    GOOGLE_LOG( INFO ) << "start to connect to server " << addr << ":" << port;
    
    shared_ptr<pbrpcpp::UdpRpcChannel> channel( new pbrpcpp::UdpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    client.echo( &controller, &request,&response, NULL, 5000 );
    
    EXPECT_FALSE( controller.Failed() );
    EXPECT_EQ( response.response(), "hello, world");
    testServer->stop();
}

TEST( UdpRpcTest, AsyncSuccess ) {
    shared_ptr<pbrpcpp::UdpRpcServer> rpcServer( new pbrpcpp::UdpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::UdpRpcServer> > testServer( new EchoTestServer<pbrpcpp::UdpRpcServer>( rpcServer, 1 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    string addr, port;
    
    while( ! rpcServer->getLocalEndpoint(addr, port ));

    GOOGLE_LOG( INFO ) << "start to connect to server " << addr << ":" << port;
    shared_ptr<pbrpcpp::UdpRpcChannel> channel( new pbrpcpp::UdpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    boost::mutex respMutex;
    boost::condition_variable cond;
    
    client.echo( &controller, &request,&response, NewCallback( responseReceived, &cond), 5000 );
    
    {
        boost::unique_lock<boost::mutex> lock(respMutex);
        cond.wait( lock );
    }
    EXPECT_FALSE( controller.Failed() );
    EXPECT_EQ( response.response(), "hello, world");
    testServer->stop();
}

TEST( UdpRpcTest, SyncTimeout ) {
    shared_ptr<pbrpcpp::UdpRpcServer> rpcServer( new pbrpcpp::UdpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::UdpRpcServer> > testServer( new EchoTestServer<pbrpcpp::UdpRpcServer>( rpcServer, 5 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    string addr, port;
    
    while( ! rpcServer->getLocalEndpoint(addr, port ));
    
    shared_ptr<pbrpcpp::UdpRpcChannel> channel( new pbrpcpp::UdpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    client.echo( &controller, &request,&response, NULL, 100 );
    
    EXPECT_TRUE( controller.Failed() );
    testServer->stop();
}

TEST( UdpRpcTest, AsyncTimeout ) {
    shared_ptr<pbrpcpp::UdpRpcServer> rpcServer( new pbrpcpp::UdpRpcServer( "localhost", "0" ) );
    shared_ptr<EchoTestServer<pbrpcpp::UdpRpcServer> > testServer( new EchoTestServer<pbrpcpp::UdpRpcServer> ( rpcServer, 5 ) );
    
    testServer->start();
    
    pbrpcpp::RpcController controller;
    
    string addr, port;
    
    while( ! rpcServer->getLocalEndpoint(addr, port ));
    
    shared_ptr<pbrpcpp::UdpRpcChannel> channel( new pbrpcpp::UdpRpcChannel( addr, port ) );
    EchoTestClient client( channel );
    echo::EchoRequest request;
    echo::EchoResponse response;
    
    request.set_message("hello, world");
    boost::mutex respMutex;
    boost::condition_variable cond;
    
    client.echo( &controller, &request,&response, NewCallback( responseReceived, &cond), 2000 );
    {
        boost::unique_lock<boost::mutex> lock(respMutex);
        cond.wait( lock );
    }
    EXPECT_TRUE( controller.Failed() );
    testServer->stop();
}

struct MessageReceiver
{
  void operator()(const string& msg)
  {
    msg_ = msg;
    cond_.notify_all();
  }
  string msg_;
  boost::mutex respMutex_;
  boost::condition_variable cond_;
};

TEST( ShmRpcTest, ShmConnection )
{
  MessageReceiver serverReceiver;
  
  pbrpcpp::ShmConnection client;
  pbrpcpp::ShmConnection server;
  
  server.startCreate("ShmRpcTest_ShmConnection", boost::ref(serverReceiver));
  client.startConnect("ShmRpcTest_ShmConnection");
  
  EXPECT_TRUE(server.isConnected());
  EXPECT_TRUE(client.isConnected());
   
  client.sendMessage("clientToServerMessage");
  {
    boost::unique_lock<boost::mutex> lock(serverReceiver.respMutex_);
    serverReceiver.cond_.wait( lock );
  }
  string expected("clientToServerMessage");
  EXPECT_EQ( 0, serverReceiver.msg_.compare(expected) );
}

TEST( ShmRpcTest, SyncSuccess ) {
  const string segment = "ShmRpcTest_SyncSuccess";
  shared_ptr<pbrpcpp::ShmRpcServer> rpcServer( new pbrpcpp::ShmRpcServer( segment ) );
  shared_ptr< EchoTestServer<pbrpcpp::ShmRpcServer> > testServer( new EchoTestServer<pbrpcpp::ShmRpcServer>( rpcServer, 1 ) );
  
  testServer->start();
  boost::this_thread::sleep(boost::posix_time::milliseconds(200));
  
  pbrpcpp::RpcController controller;
  GOOGLE_LOG( INFO ) << "start to connect to server " << segment;
  
  shared_ptr<pbrpcpp::ShmRpcChannel> channel( new pbrpcpp::ShmRpcChannel( segment ) );
  EchoTestClient client( channel );
  echo::EchoRequest request;
  echo::EchoResponse response;
  
  request.set_message("hello, world");
  client.echo( &controller, &request,&response, NULL, 5000 );
  
  EXPECT_FALSE( controller.Failed() );
  EXPECT_EQ( response.response(), "hello, world");
  testServer->stop();
}

void emptyLogHandler(google::protobuf::LogLevel level, const char* filename, int line,
                        const std::string& message)
{
  std::cout << message << std::endl;
}


/*
 * 
 */
int main(int argc, char** argv) {   
    google::protobuf::SetLogHandler( &emptyLogHandler );
    InitGoogleTest(&argc, argv);
    int ret_val = RUN_ALL_TESTS();
    return ret_val;
}

