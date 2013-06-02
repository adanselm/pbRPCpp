/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef ECHOTESTCLIENT_HPP
#define	ECHOTESTCLIENT_HPP

#include "echo.pb.h"
#include "BaseRpcChannel.hpp"
#include <boost/smart_ptr.hpp>

class EchoTestClient {
public:
    EchoTestClient( const shared_ptr<pbrpcpp::BaseRpcChannel>& channel );
  
    void echo( google::protobuf::RpcController* controller,
                              const echo::EchoRequest* request,
                              echo::EchoResponse* response,
                              Closure* done,
                              int timeoutMillis = 0 );
private:
    shared_ptr<pbrpcpp::BaseRpcChannel> channel_;
};

#endif	/* ECHOTESTCLIENT_HPP */

