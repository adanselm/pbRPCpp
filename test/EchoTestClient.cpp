/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "EchoTestClient.hpp"

EchoTestClient::EchoTestClient(shared_ptr<pbrpcpp::BaseRpcChannel> channel)
: channel_(channel) {
}

void EchoTestClient::echo(google::protobuf::RpcController* controller,
        const echo::EchoRequest* request,
        echo::EchoResponse* response,
        Closure* done,
        int timeoutMillis) {
    channel_->setRequestTimeout(timeoutMillis);
    echo::EchoService::Stub stub(channel_.get());
    stub.Echo(controller, request, response, done);
}
