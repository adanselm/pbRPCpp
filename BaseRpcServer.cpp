/* 
 * File:   MemRpcServer.h
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
    :stop_( false )            
    {
        requestProcThreads_.create_thread( boost::bind( &BaseRpcServer::takeAndProcessMsg, this ));
    }
    void BaseRpcServer::Export( Service* service ) {
        if( service ) {
                services_[ service->GetDescriptor() ] = service;
        }
    }
    
    void BaseRpcServer::Shutdown() {
        if( stop_ ) {
            return;
        }
        
        stop_ = true;
        requestProcThreads_.interrupt_all();
        requestProcThreads_.join_all();
    }
    
    void BaseRpcServer::messageReceived( int clientId, const string& msg ) {
        clientMsgs_.add( new ClientMsg( clientId, msg ) );
    }
    
    void BaseRpcServer::takeAndProcessMsg() {
        while( !stop_ ) {
            try {
                ClientMsg* clientMsg = clientMsgs_.take();
                if( clientMsg ) {
                    processMessage( clientMsg->clientId, clientMsg->msg );
                    delete clientMsg;
                }
            }catch( ... ) {

            }
        }
        
    }
    
    void BaseRpcServer::processMessage( int clientId, const string& msg ) {
         try {
            istringstream in( msg );
            
            int msgType = Util::readInt( in );
            switch( msgType ) {
                case RpcMessage::REQUEST_MSG:
                {
                    GOOGLE_LOG( INFO ) << "a request is received";
                    string callId;
                    Message* requestMsg = 0;
                    const MethodDescriptor* descriptor = 0;
                    
                    RpcMessage::parseRequestFrom( in, callId, descriptor, requestMsg );
                        
                    requestReceived( clientId, callId, descriptor, requestMsg );
                }
                    break;
            }
        }catch( const std::exception& ex) {
            GOOGLE_LOG( ERROR ) << "catch exception:" << ex.what();
        }catch( ... ) {
            GOOGLE_LOG( ERROR ) << "catch unknown exception";
        }
    }
    
    
    void BaseRpcServer::requestReceived( int clientId, 
                    const string& callId,
                    const MethodDescriptor* method,
                    Message* requestMsg ) {
        try {
            Service* pService = getService( method );
            if( pService ) {
                GOOGLE_LOG( INFO ) << "find service by method:" << method->full_name();
                Message* response = pService->GetResponsePrototype( method ).New();
                RpcController* controller = new RpcController();

                Request* pRequest = new Request( clientId, callId, controller, requestMsg, response );
                addRequest( pRequest );
                pService->CallMethod( method, 
                        controller,
                        requestMsg,
                        response,
                        NewCallback( this, &BaseRpcServer::requestFinished, pRequest ) );

            } else {
                GOOGLE_LOG( INFO ) << "fail to find service by method:" << method->full_name();
                RpcController controller;
                controller.SetFailed("fail to find the service in the server");
                sendResponse( clientId, callId, &controller, 0 );
            }
        }catch( ... ) {
            
        }
    }
    
    void BaseRpcServer::cancelRequest( int clientId, const string& callId ) {
        Request* request = removeRequest( clientId, callId );
        if( request ) {
            RpcController* controller = new RpcController();
            controller->cancel();
            sendResponse( clientId, callId, controller, 0 );
        }
    }
    
    void BaseRpcServer::sendResponse( int clientId, const string& callId, RpcController* controller, Message* responseMsg ) {
        ostringstream out;
        
        RpcMessage::serializeResponse( callId, *controller, responseMsg, out );
        
        sendResponse( clientId, out.str() );
    }    

    Service* BaseRpcServer::getService( const MethodDescriptor* method ) {
        return services_[ method->service() ];
    }    

    void BaseRpcServer::requestFinished( Request* request ) {
        
        if( request ) {
            if( removeRequest( request->clientId, request->callId ) == request ) {
                GOOGLE_LOG( INFO ) << "request process is finished, start to send response";
                sendResponse( request->clientId, request->callId, request->controller, request->responseMsg );
            } else {
                GOOGLE_LOG( ERROR ) << "request process is finished, but can't find the request";
            }
        }
        
        delete request;
    }
    
    void BaseRpcServer::addRequest( BaseRpcServer::Request* request ) {
        boost::lock_guard< boost::mutex > guard( requestMutex_ );
        
        BOOST_VERIFY( requests_.insert( map< ClientCallId, Request*>::value_type( ClientCallId( request->clientId, request->callId ), request ) ).second );            
    }
    
    BaseRpcServer::Request* BaseRpcServer::removeRequest( int clientId, const string& callId ) {
        boost::lock_guard< boost::mutex > guard( requestMutex_ );
        Request* req = 0;
        
        map< ClientCallId, Request*>::iterator iter = requests_.find( ClientCallId( clientId, callId ) );
        if( iter != requests_.end() ) {
            req = iter->second;
            requests_.erase( iter );
        }
        return req;
    }

}


