#include "EchoTestServer.hpp"

EchoServiceImpl::EchoServiceImpl(int echoDelay, const string& failReason)
: echoDelay_(echoDelay),
failReason_(failReason) {

}

void EchoServiceImpl::Echo(::google::protobuf::RpcController* controller,
        const ::echo::EchoRequest* request,
        ::echo::EchoResponse* response,
        ::google::protobuf::Closure* done) {
    if (failReason_.empty()) {
        response->set_response(request->message());
    } else {
        controller->SetFailed(failReason_);
    }

    if (echoDelay_ > 0) {
        ::sleep(echoDelay_);
    }

    done->Run();
}




