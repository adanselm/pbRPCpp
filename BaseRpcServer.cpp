/* 
 * File:   BaseRpcServer.h
 * Author: Steven
 *
 * Created on March 17, 2013, 11:18 PM
 */


#include "BaseRpcServer.hpp"
#include "RpcMessage.hpp"
#include "BaseRpcChannel.hpp"
#include <sstream>

using std::istringstream;
using std::ostringstream;
using google::protobuf::NewCallback;

namespace pbrpcpp {

    BaseRpcServer::BaseRpcServer()
    : stop_(false),
      processingRequests_( 0 )
    {
        requestProcThreads_.create_thread(boost::bind(&BaseRpcServer::takeAndProcessMsg, this, boost::ref( clientRequestMsgs_ )));
        requestProcThreads_.create_thread(boost::bind(&BaseRpcServer::takeAndProcessMsg, this, boost::ref( clientCancelMsgs_ )));
    }

    BaseRpcServer::~BaseRpcServer() {
        requestProcThreads_.interrupt_all();
        requestProcThreads_.join_all();        
    }
    
    void BaseRpcServer::Export(Service* service) {
        if (service) {
            services_[ service->GetDescriptor() ] = service;
        }
    }


    void BaseRpcServer::messageReceived(int clientId, const string& msg) {
        try {
            size_t pos = 0;

            switch (Util::readInt(msg, pos)) {
                case RpcMessage::REQUEST_MSG:
                    clientRequestMsgs_.add(new ClientMsg(clientId, msg));
                    break;
                case RpcMessage::CANCEL_MSG:
                    clientCancelMsgs_.add(new ClientMsg(clientId, msg));
                    break;

            }

        } catch (const std::exception& ex) {
            GOOGLE_LOG( ERROR ) << "catch exception:" << ex.what();
        } catch (...) {
            GOOGLE_LOG( ERROR ) << "catch unknown exception";
        }
    }

    void BaseRpcServer::takeAndProcessMsg( Queue< ClientMsg* >& msgQueue ) {
        for( ; ; ) {
            try {
                ClientMsg* clientMsg = msgQueue.take();
                if (clientMsg) {
                    processMessage(clientMsg->clientId, clientMsg->msg);
                    delete clientMsg;
                }
            } catch ( const boost::thread_interrupted& ex ) {
                GOOGLE_LOG( INFO ) << "thread interrupted";
                break;
            }catch(...) {
            }
        }

    }

    void BaseRpcServer::processMessage(int clientId, const string& msg) {
        try {
            istringstream in(msg);

            int msgType = Util::readInt(in);
            switch (msgType) {
                case RpcMessage::REQUEST_MSG:
                {
                    GOOGLE_LOG(INFO) << "a request is received";
                    string callId;
                    Message* requestMsg = 0;
                    const MethodDescriptor* descriptor = 0;

                    RpcMessage::parseRequestFrom(in, callId, descriptor, requestMsg);

                    requestReceived(clientId, callId, descriptor, requestMsg);
                }
                    break;
                case RpcMessage::CANCEL_MSG:
                {
                    GOOGLE_LOG(INFO) << "a cancel request is received";
                    string callId;

                    RpcMessage::parseCancelFrom(in, callId);
                    cancelRequest(clientId, callId);
                }
                    break;
            }
        } catch (const std::exception& ex) {
            GOOGLE_LOG(ERROR) << "catch exception:" << ex.what();
        } catch (...) {
            GOOGLE_LOG(ERROR) << "catch unknown exception";
        }
    }

    void BaseRpcServer::requestReceived(int clientId,
            const string& callId,
            const MethodDescriptor* method,
            Message* requestMsg) {
        try {
            Service* pService = getService(method);
            if (pService) {
                GOOGLE_LOG(INFO) << "find service by method:" << method->full_name() << ", execute request with callId:" << callId << " from client " << clientId;
                Message* response = pService->GetResponsePrototype(method).New();
                RpcController* controller = new RpcController();

                Request* pRequest = new Request(clientId, callId, controller, requestMsg, response);
                addRequest(pRequest);
                processingRequests_ ++;
                pService->CallMethod(method,
                        controller,
                        requestMsg,
                        response,
                        NewCallback(this, &BaseRpcServer::requestFinished, pRequest));

            } else {
                GOOGLE_LOG(INFO) << "fail to find service by method:" << method->full_name();
                RpcController controller;
                controller.SetFailed("fail to find the service in the server");
                sendResponse(clientId, callId, &controller, 0);
            }
        } catch (...) {

        }
    }

    void BaseRpcServer::cancelRequest(int clientId, const string& callId) {
        Request* request = removeRequest(clientId, callId);
        //at this time the require can't be deleted
        if (request) {
            GOOGLE_LOG( INFO ) << "cancel request with callId " << callId << " from client " << clientId;
            RpcController controller;
            controller.cancel();
            sendResponse(clientId, callId, &controller, 0);            
        } else {
            GOOGLE_LOG( INFO ) << "the request is already finished, cancel call " << callId << " is ignored from client " << clientId;
        }               
    }

    void BaseRpcServer::sendResponse(int clientId, const string& callId, RpcController* controller, Message* responseMsg) {
        ostringstream out;

        RpcMessage::serializeResponse(callId, *controller, responseMsg, out);

        sendResponse(clientId, out.str());
    }

    Service* BaseRpcServer::getService(const MethodDescriptor* method) {
        return services_[ method->service() ];
    }

    void BaseRpcServer::requestFinished(Request* request) {

        processingRequests_ --;
        
        //this time, the request must be deleted
        if (request) {
            if (removeRequest(request->clientId, request->callId) == request) {
                GOOGLE_LOG(INFO) << "request process is finished, start to send response of request " << request->callId << " to client " << request->clientId;
                sendResponse(request->clientId, request->callId, request->controller, request->responseMsg);
            } else {
                GOOGLE_LOG(ERROR) << "request process is finished, but can't find the request";
            }
        }

        delete request;
    }

    void BaseRpcServer::addRequest(BaseRpcServer::Request* request) {
        BOOST_VERIFY(requests_.insert(ClientCallId(request->clientId, request->callId), request).second);
    }

    BaseRpcServer::Request* BaseRpcServer::removeRequest(int clientId, const string& callId) {
        return requests_.erase(ClientCallId(clientId, callId));
    }

}//end name space pbrpcpp


