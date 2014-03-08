/**
*          Copyright Springbeats Sarl 2013.
* Distributed under the Boost Software License, Version 1.0.
*    (See accompanying file ../LICENSE_1_0.txt or copy at
*          http://www.boost.org/LICENSE_1_0.txt)
*/
#include "BaseRpcChannel.hpp"
#include "RpcController.hpp"
#include "MethodCallIDGenerator.hpp"
#include "RpcMessage.hpp"
#include "Timer.hpp"
#include "Queue.hpp"
#include "ThreadSafeMap.hpp"
#include "Util.hpp"
#include <sstream>
#include <boost/bind.hpp>

using std::istringstream;

namespace pbrpcpp {

	BaseRpcChannel::BaseRpcChannel( )
		:timeoutMillis_( 0 ),
		timer_( new Timer<string>() ),
		responses_( new Queue<string>() ),
		waitingResponses_( new ThreadSafeMap< string, shared_ptr<ResponseParam> >() )
	{        
		//create a thread to process all the responses
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
			//each call will allocate a unique callId for parallel processing reason
			string callId = MethodCallIDGenerator::generateID();

			if( controller ) {
				RpcController* pRpcController = dynamic_cast<RpcController*>( controller );
				if( pRpcController ) {
					pRpcController->setStartCancelCallback( boost::bind(&BaseRpcChannel::startCancel, this, callId ) );
				}
			}

			bool completed = false;
			shared_ptr<ResponseParam> respParam( new ResponseParam( controller, response, done, (done == 0)?(&completed):0 ) );

			(*waitingResponses_)[ callId ] = respParam ;

			//serialize the message
			ostringstream out;

			RpcMessage::serializeRequest( callId, *method, *request, out );

			//start a timer if timeout is enabled
			if( timeoutMillis_ > 0 ) {
				timer_->add( callId, timeoutMillis_, boost::bind( &BaseRpcChannel::handleRequestTimeout, this, callId ) );
			}

			//send the serialized message to server
			sendMessage( out.str(), boost::bind( &BaseRpcChannel::messageSent, this, _1, _2, callId ) );

			// if sync call, waiting for complete
			if( done == 0 ) {
				boost::unique_lock<boost::mutex> lock(mMutex);
 				while(!completed)
 				{
 					mCondVariable.wait(lock);
 				}

			}
	}

	void BaseRpcChannel::messageSent( bool success, const string& reason, string callId) {
		//if fail to send message to the server
		if( ! success ) {
			shared_ptr<ResponseParam> respParam = waitingResponses_->erase( callId );
			if( respParam ) {
				timer_->cancel( callId );
				RpcController* pController = dynamic_cast<RpcController*>( respParam->controller );
				if( pController ) {
					pController->SetFailed( reason );
				}

				if( respParam->completed ) {
					{
						boost::lock_guard<boost::mutex> lock(mMutex);
						*(respParam->completed) = true;
					}
					mCondVariable.notify_all();
					
					
				}

				if (respParam->done) {
					respParam->done->Run();
				}

				if( pController ) {
					pController->complete();
				}
			}
		}
	}

	void BaseRpcChannel::startCancel( string callId ) {

		//if the request is still on-going, send cancel request to server
		if( waitingResponses_->contains( callId ) ) {
			ostringstream out;

			RpcMessage::serializeCancel( callId, out );

			sendMessage( out.str(), boost::bind( &BaseRpcChannel::messageSent, this, _1, _2, callId )  );
		}
	}

	void BaseRpcChannel::responseReceived( const string& responseMsg ) {
		responses_->add( responseMsg );
	}

	void BaseRpcChannel::takeAndProcessResponse() {
		for( ; ; ) {
			try {
				string resp = responses_->take();

				if( !resp.empty() ) {
					processResponse( resp );
				}
			}catch( const boost::thread_interrupted& ex ) {
				//                GOOGLE_LOG( INFO ) << "thread interrupted";
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
					shared_ptr<Message> response;
					//parse the response
					RpcMessage::parseResponseFrom( in, callId, controller, response );
					GOOGLE_LOG( INFO ) << "received a response message, callId:" << callId;
					//if the response is still not timeout
					shared_ptr<ResponseParam> respParam = waitingResponses_->erase( callId );
					if( respParam ) {                            
						timer_->cancel( callId );
						RpcController* pController = dynamic_cast<RpcController*>( respParam->controller );
						if( pController ) {
							copyController( *pController, controller );
						}
						if( respParam->response && response ) {
							copyMessage( *(respParam->response), *response );
						}

						if( respParam->completed ) {
							
							{
								boost::lock_guard<boost::mutex> lock(mMutex);
								*(respParam->completed) = true;
							}
							mCondVariable.notify_all();
						}

						if( respParam->done ) {
							respParam->done->Run();
						}

						if( pController ) {
							pController->complete();
						}

					}
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
		dest.canceled_ = src.canceled_;
	}

	void BaseRpcChannel::copyMessage( Message& dest, const Message& src ) {
		if( dest.GetDescriptor() == src.GetDescriptor() ) {
			dest.CopyFrom( src );
		}
	}



	bool BaseRpcChannel::isCallCompleted( const string& callId )  {
		return !waitingResponses_->contains( callId );
	}

	void BaseRpcChannel::handleRequestTimeout( string callId ) {
		shared_ptr<ResponseParam> respParam = waitingResponses_->erase( callId );
		if( respParam ) {
			RpcController* pController = dynamic_cast<RpcController*> (respParam->controller);
			if (pController) {
				pController->SetFailed("request timeout");
			}

			if (respParam->completed) {
				{
					boost::lock_guard<boost::mutex> lock(mMutex);
					*(respParam->completed) = true;
				}
				mCondVariable.notify_all();
			}

			if (respParam->done) {
				respParam->done->Run();
			}

			if( pController ) {
				pController->complete();
			}
		}
	}

}//end name space pbrpcpp


