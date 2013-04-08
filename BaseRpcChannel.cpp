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
    :timeoutMillis_( 0 )
    {        
        responseProcThreads_.create_thread( boost::bind( &BaseRpcChannel::takeAndProcessResponse, this ));
    }

    BaseRpcChannel::~BaseRpcChannel() {
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

        waitingResponses_[ callId ] = respParam ;

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

    void BaseRpcChannel::messageSent( bool success, const string& reason, string callId ) {
        if( ! success ) {
            ResponseParam* respParam = waitingResponses_.erase( callId );
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

        //if it is on-going
        if( waitingResponses_.contains( callId ) ) {
            ostringstream out;

            RpcMessage::serializeCancel( callId, out );

            sendMessage( out.str(), boost::bind( &BaseRpcChannel::messageSent, this, _1, _2, callId )  );
        }
    }

    void BaseRpcChannel::responseReceived( const string& responseMsg ) {
        responses_.add( responseMsg );
    }
    
    void BaseRpcChannel::takeAndProcessResponse() {
        for( ; ; ) {
            try {
                string resp = responses_.take();
                
                if( !resp.empty() ) {
                    processResponse( resp );
                }
            }catch( const boost::thread_interrupted& ex ) {
                GOOGLE_LOG( INFO ) << "thread interrupted";
                break;
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
                            string callId;
                            RpcController controller;
                            Message* response = 0;
                            RpcMessage::parseResponseFrom( in, callId, controller, response );
                            GOOGLE_LOG( INFO ) << "received a response message, callId:" << callId;
                            ResponseParam* respParam = waitingResponses_.erase( callId );
                            if( respParam ) {                            
                                timer_.cancel( callId );
                                RpcController* pController = dynamic_cast<RpcController*>( respParam->controller );
                                if( pController ) {
                                    copyController( *pController, controller );
                                }
                                if( respParam->response && response ) {
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

            }catch( const std::exception& ex) {
                GOOGLE_LOG( ERROR ) << "catch exception:" << ex.what() ;
            } catch( ... ) {
                GOOGLE_LOG( ERROR ) << "catch unknown exception";
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
        return !waitingResponses_.contains( callId );
    }
    
    void BaseRpcChannel::handleRequestTimeout( string callId ) {
        ResponseParam* respParam = waitingResponses_.erase( callId );
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

}//end name space pbrpcpp


