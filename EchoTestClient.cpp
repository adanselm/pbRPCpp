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
