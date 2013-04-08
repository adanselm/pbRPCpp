/* 
 * File:   BaseRpcChannel.cpp
 * Author: Steven
 *
 * Created on March 17, 2013, 11:14 PM
 */
#include "BaseRpcChannel.hpp"
#include "MethodCallIDGenerator.hpp"
#include "RpcMessage.hpp"
#include <sstream>

using std::istringstream;

namespace pbrpcpp {
    
    BaseRpcChannel::BaseRpcChannel( )
    :stop_( false ),
    timeoutMillis_( 0 )
    {        
        responseProcThreads_.create_thread( boost::bind( &BaseRpcChannel::takeAndProcessResponse, this ));
    }

    BaseRpcChannel::~BaseRpcChannel() {
        stop_ = true;
        responseProcThreads_.interrupt_all();
        responseProcThreads_.join_all();
    }

    void BaseRpcChannel::CallMethod(const MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                          const Message* request,
                          Message* response,
                          Closure* done) {        
        string callId = MethodCallIDGenerator::generateID();

        if( controller ) {
            RpcController* pRpcController = dynamic_cast<RpcController*>( controller );
            if( pRpcController ) {
                pRpcController->setStartCancelCallback( NewCallback(this, &BaseRpcChannel::startCancel, callId ) );
            }
        }

        bool* completed = ( done == 0 )?( new bool( false ) ): 0;        
        ResponseParam* respParam = new ResponseParam( controller, response, done, completed );

        {
            boost::lock_guard< boost::mutex > guard( respMutex_ );
            waitingResponses_[ callId ] = respParam;
        }

        ostringstream out;

        RpcMessage::serializeRequest( callId, *method, *request, out );
        
        if( timeoutMillis_ > 0 ) {
            timer_.add( callId, timeoutMillis_, boost::bind( &BaseRpcChannel::handleRequestTimeout, this, callId ) );
        }
        
        sendMessage( out.str(), boost::bind( &BaseRpcChannel::messageSent, this, _1, _2, callId ) );

        if( done == 0 ) {

            while( !(*completed) );

            delete completed;          
        }
    }

    BaseRpcChannel::ResponseParam* BaseRpcChannel::removeRespParam( const string& callId ) {
        boost::lock_guard< boost::mutex > guard( respMutex_ );

        map< string, ResponseParam* >::iterator iter = waitingResponses_.find( callId );

        if( iter == waitingResponses_.end() ) {
            return 0;
        }

        ResponseParam* respParam = iter->second;
        waitingResponses_.erase( iter );
        return respParam;
    }
    
    void BaseRpcChannel::messageSent( bool success, const string& reason, string callId ) {
        if( ! success ) {
            ResponseParam* respParam = removeRespParam( callId );
            if( respParam ) {
                timer_.cancel( callId );
                RpcController* pController = dynamic_cast<RpcController*>( respParam->controller );
                if( pController ) {
                    pController->SetFailed( reason );
                }
                
                if( respParam->completed ) {
                    *(respParam->completed) = true;
                }

                if (respParam->done) {
                    respParam->done->Run();
                }

                delete respParam;
                
            }
        }
    }

    void BaseRpcChannel::startCancel( string callId ) {

        if( !isCallCompleted( callId ) ) {
            ostringstream out;

            RpcMessage::serializeCancel( callId, out );

            sendMessage( out.str(), boost::bind( &BaseRpcChannel::messageSent, this, _1, _2, callId )  );

        }
    }

    void BaseRpcChannel::responseReceived( const string& responseMsg ) {
        responses_.add( responseMsg );
    }
    
    void BaseRpcChannel::takeAndProcessResponse() {
        while( !stop_ ) {
            try {
                string resp = responses_.take();
                
                if( !resp.empty() ) {
                    processResponse( resp );
                }
            }catch( ... ) {
                
            }
        }
    }
    void BaseRpcChannel::processResponse( const string& responseMsg ) {
        try {
                    istringstream in( responseMsg );
                    int msgType = Util::readInt( in );

                    switch( msgType ) {
                        case RpcMessage::RESPONSE_MSG: 
                        {
                            GOOGLE_LOG( INFO ) << "received a response message";
                            string callId;
                            RpcController controller;
                            Message* response = 0;
                            RpcMessage::parseResponseFrom( in, callId, controller, response );
                            ResponseParam* respParam = removeRespParam( callId );
                            if( respParam ) {                            
                                timer_.cancel( callId );
                                RpcController* pController = dynamic_cast<RpcController*>( respParam->controller );
                                if( pController ) {
                                    copyController( *pController, controller );
                                }
                                if( respParam->response ) {
                                    copyMessage( *(respParam->response), *response );
                                }

                                if( respParam->completed ) {
                                    *(respParam->completed) = true;
                                }

                                if( respParam->done ) {
                                    respParam->done->Run();
                                }

                                delete respParam;
                            }
                            delete response;
                        }
                        break;
                    }

            }catch( ... ) {

            }
    }

    

    void BaseRpcChannel::copyController( RpcController& dest, const RpcController& src ) {
            if( src.failed_ ) {
                dest.SetFailed ( src.ErrorText() );
            }
            if( src.canceled_ ) {
                dest.cancel();
            }
    }

    void BaseRpcChannel::copyMessage( Message& dest, const Message& src ) {
        if( dest.GetDescriptor() == src.GetDescriptor() ) {
            dest.CopyFrom( src );
        }
    }

    

    bool BaseRpcChannel::isCallCompleted( const string& callId )  {
        boost::lock_guard< boost::mutex > guard( respMutex_ );

        return waitingResponses_.find( callId ) == waitingResponses_.end();
    }
    
    void BaseRpcChannel::handleRequestTimeout( string callId ) {
        ResponseParam* respParam = removeRespParam( callId );
        if( respParam ) {
            RpcController* pController = dynamic_cast<RpcController*> (respParam->controller);
            if (pController) {
                pController->SetFailed("request timeout");
            }

            if (respParam->completed) {
                *(respParam->completed) = true;
            }

            if (respParam->done) {
                respParam->done->Run();
            }

            delete respParam;

        }
    }

}


