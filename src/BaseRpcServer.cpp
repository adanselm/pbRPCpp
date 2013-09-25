/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "RpcController.hpp"
#include "BaseRpcServer.hpp"
#include "RpcMessage.hpp"
#include "BaseRpcChannel.hpp"
#include <sstream>
#include "Queue.hpp"
#include "ThreadSafeMap.hpp"
#include "AtomicInteger.hpp"
#include "Util.hpp"

using std::istringstream;
using std::ostringstream;
using google::protobuf::NewCallback;

namespace pbrpcpp {

    BaseRpcServer::BaseRpcServer()
    : stop_(false),
      processingRequests_( new AtomicInteger<int>(0) ),
      requests_( new ThreadSafeMap<ClientCallId, shared_ptr<Request> >() ),
      clientRequestMsgs_( new Queue<shared_ptr<ClientMsg> >() ),
      clientCancelMsgs_( new Queue<shared_ptr<ClientMsg> >() )
    {
        //create two threads: one to process the normal request, another to process cancel request
        requestProcThreads_.create_thread(boost::bind(&BaseRpcServer::takeAndProcessMsg, this, boost::ref( *clientRequestMsgs_ )));
        requestProcThreads_.create_thread(boost::bind(&BaseRpcServer::takeAndProcessMsg, this, boost::ref( *clientCancelMsgs_ )));
    }

    BaseRpcServer::~BaseRpcServer() {
        stop_ = true;
        requestProcThreads_.interrupt_all();
        requestProcThreads_.join_all();        
    }
    
    void BaseRpcServer::Export(Service* service) {
        //register the service
        if (service) {
            services_[ service->GetDescriptor() ] = service;
        }
    }


    void BaseRpcServer::messageReceived(int clientId, const string& msg) {
        try {
            size_t pos = 0;

            //check the message time, ignore the message if we don't know it type
            switch (Util::readInt(msg, pos)) {
                case RpcMessage::REQUEST_MSG:
                    clientRequestMsgs_->add( shared_ptr<ClientMsg>(new ClientMsg(clientId, msg)) );
                    break;
                case RpcMessage::CANCEL_MSG:
                    clientCancelMsgs_->add( shared_ptr<ClientMsg>(new ClientMsg(clientId, msg)) );
                    break;

            }

        } catch (const std::exception& ex) {
            GOOGLE_LOG( ERROR ) << "catch exception:" << ex.what();
        } catch (...) {
            GOOGLE_LOG( ERROR ) << "catch unknown exception";
        }
    }

    void BaseRpcServer::takeAndProcessMsg( Queue< shared_ptr<ClientMsg> >& msgQueue ) {
      while( stop_ == false )
      {
        try
        {
          shared_ptr<ClientMsg> clientMsg = msgQueue.take();
          if (clientMsg)
          {
            processMessage(clientMsg->clientId, clientMsg->msg);
          }
        }
        catch( const boost::thread_interrupted& ex )
        {
          //                GOOGLE_LOG( INFO ) << "thread interrupted";
          break;
        }
        catch(...)
        {
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
                    shared_ptr<Message> requestMsg;
                    const MethodDescriptor* descriptor = 0;

                    RpcMessage::parseRequestFrom(in, callId, descriptor, requestMsg);

                    if( ! stop_ )
                      requestReceived(clientId, callId, descriptor, requestMsg);
                }
                    break;
                case RpcMessage::CANCEL_MSG:
                {
                    GOOGLE_LOG(INFO) << "a cancel request is received";
                    string callId;

                    RpcMessage::parseCancelFrom(in, callId);
                    if( ! stop_ )
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
            shared_ptr<Message> requestMsg) {
        try {
            Service* pService = getService(method);
            if (pService) {
                GOOGLE_LOG(INFO) << "find service by method:" << method->full_name() << ", execute request with callId:" << callId << " from client " << clientId;
                shared_ptr<Message> response( pService->GetResponsePrototype(method).New() );
                shared_ptr<RpcController> controller( new RpcController() );

                shared_ptr<Request> pRequest( new Request(clientId, callId, controller, requestMsg, response) );
                addRequest(pRequest);
                (*processingRequests_)++;
                pService->CallMethod(method,
                        controller.get(),
                        requestMsg.get(),
                        response.get(),
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
        shared_ptr<Request> request = removeRequest(clientId, callId);
        if (request) {
            GOOGLE_LOG( INFO ) << "cancel request with callId " << callId << " from client " << clientId;
            RpcController controller;
            controller.canceled_ = true;
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

    void BaseRpcServer::requestFinished(shared_ptr<BaseRpcServer::Request> request) {

        (*processingRequests_)--;
        
        if (request) {
            if (removeRequest(request->clientId, request->callId) == request) {
                GOOGLE_LOG(INFO) << "request process is finished, start to send response of request " << request->callId << " to client " << request->clientId;
                sendResponse(request->clientId, request->callId, request->controller.get(), request->responseMsg.get());
            } else {
                GOOGLE_LOG(ERROR) << "request process is finished, but can't find the request with id " << request->callId << "," << *processingRequests_ << " request is running";
            }
        }
    }
  
    int BaseRpcServer::getProcessingRequests() const {
      return *processingRequests_;
    }

    void BaseRpcServer::addRequest(shared_ptr<BaseRpcServer::Request> request) {
        BOOST_VERIFY(requests_->insert(ClientCallId(request->clientId, request->callId), request).second);
    }

    shared_ptr<BaseRpcServer::Request> BaseRpcServer::removeRequest(int clientId, const string& callId) {
        return requests_->erase(ClientCallId(clientId, callId));
    }

}//end name space pbrpcpp


