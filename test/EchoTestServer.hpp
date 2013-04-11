/* 
 * File:   EchoTestServer.hpp
 * Author: stou
 *
 * Created on April 6, 2013, 5:26 PM
 */

#ifndef ECHOTESTSERVER_HPP
#define	ECHOTESTSERVER_HPP

#include "echo.pb.h"
#include "BaseRpcServer.hpp"
#include "Queue.hpp"
#include <string>

using std::string;

class EchoServiceImpl : public echo::EchoService {
public:
    EchoServiceImpl(int echoDelay, const string& failReason = string());
    virtual void Echo(::google::protobuf::RpcController* controller,
            const ::echo::EchoRequest* request,
            ::echo::EchoResponse* response,
            ::google::protobuf::Closure* done);
private:
    //echo delay in seconds
    int echoDelay_;

    //if not empty, send an error response
    string failReason_;
};

template< typename RpcServer >
class EchoTestServer {
public:

    EchoTestServer(shared_ptr<RpcServer> rpcServer,
            int echoDelay,
            bool exportEchoService = true,
            const string& failReason = string())
    : rpcServer_(rpcServer),
    echoService_(echoDelay, failReason) {
        if (exportEchoService) {
            rpcServer_->Export(&echoService_);
        }
    }

    void start() {
        serverThread_ = boost::thread(boost::bind(&EchoTestServer::run, this));
    }

    void stop() {
        rpcServer_->Shutdown();
        serverThread_.join();
    }
private:

    void run() {
        rpcServer_->Run();
    }
private:
    shared_ptr<RpcServer> rpcServer_;
    EchoServiceImpl echoService_;
    boost::thread serverThread_;
};
#endif	/* ECHOTESTSERVER_HPP */

