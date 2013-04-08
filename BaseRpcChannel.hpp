/* 
 * File:   BaseRpcChannel.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 5:43 PM
 */

#ifndef BASERPCCHANNEL_HPP
#define	BASERPCCHANNEL_HPP


#include <string>
#include <map>
#include <google/protobuf/service.h>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "Timer.hpp"
#include "RpcController.hpp"
#include "Queue.hpp"

using boost::shared_ptr;
using std::map;
using std::string;
using google::protobuf::MethodDescriptor;
using google::protobuf::Message;
using google::protobuf::RpcChannel;
using google::protobuf::Closure;

namespace pbrpcpp {
    
    class BaseRpcChannel: public RpcChannel, public boost::noncopyable {
    public:    
        BaseRpcChannel();
        virtual ~BaseRpcChannel();
        
        virtual void CallMethod(const MethodDescriptor* method,
                              google::protobuf::RpcController* controller,
                              const Message* request,
                              Message* response,
                              Closure* done);
        /**
         * set the request timeout
         * 
         * @param timeoutMillis the request timeout in milliseconds
         */
        void setRequestTimeout( int timeoutMillis ) {
            timeoutMillis_ = timeoutMillis;
        }
    protected:
        void responseReceived( const string& responseMsg );
        virtual void sendMessage( const string& msg, boost::function< void (bool, const string&) > resultCb ) = 0;
    private:
        struct ResponseParam {
            ResponseParam( google::protobuf::RpcController* _controller, Message* _response, Closure* _done, bool* _completed )
            :controller( _controller ),
            response( _response ),
            done( _done ),
            completed( _completed )
            {
            }            
            google::protobuf::RpcController* controller;
            Message* response;
            Closure* done;
            bool* completed;
        };

        void takeAndProcessResponse();
        void processResponse( const string& responseMsg);
        void messageSent( bool success, const string& reason, string callId );
        ResponseParam* removeRespParam( const string& callId );
        void startCancel( string callId );
        void copyController( RpcController& dest, const RpcController& src );
        void copyMessage( Message& dest, const Message& src );
        bool isCallCompleted( const string& callId );
        void handleRequestTimeout( string callId );
        
    protected:
        volatile bool stop_;
    private:
        int timeoutMillis_;
        boost::mutex respMutex_;
        map< string, ResponseParam* > waitingResponses_;
        Timer<string> timer_;
        Queue<string> responses_;
        boost::thread_group responseProcThreads_;
    };

}

#endif	/* BASERPCCHANNEL_HPP */

