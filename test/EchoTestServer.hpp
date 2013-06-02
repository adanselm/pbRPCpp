/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef ECHOTESTSERVER_HPP
#define	ECHOTESTSERVER_HPP

#include "echo.pb.h"
#include "BaseRpcServer.hpp"
#include "RpcServiceWrapper.hpp"
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

template <class SrvT>
struct EchoServiceAdapter
{
  typedef pbrpcpp::RpcServiceWrapper<EchoServiceImpl, SrvT> type;
};



#endif	/* ECHOTESTSERVER_HPP */

