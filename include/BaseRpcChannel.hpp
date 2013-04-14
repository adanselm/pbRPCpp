/* 
 * File:   BaseRpcChannel.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 5:43 PM
 */

#ifndef BASERPCCHANNEL_HPP
#define	BASERPCCHANNEL_HPP


#include <string>
#include <google/protobuf/service.h>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

using boost::shared_ptr;
using boost::scoped_ptr;
using std::string;
using google::protobuf::MethodDescriptor;
using google::protobuf::Message;
using google::protobuf::RpcChannel;
using google::protobuf::Closure;

namespace pbrpcpp {
    class RpcController;
    template <class T> class Timer;
    template <class T> class Queue;
    template <class T, class U> class ThreadSafeMap;
    
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

        /**
         * take the received response from <code>responses_</code> and process it
         */
        void takeAndProcessResponse();
        
        /**
         * process response taken from <code>responses_</code>. this method is called
         * by takeAndProcessResponse() method
         * @param responseMsg the response message from server
         */
        void processResponse( const string& responseMsg);
        
        /**
         * the callback for the the sendMessage() method
         * 
         * @param success true if the message is sent to server successfully,false if
         * the message is failed to send to server
         * @param reason the reason if the message is failed to send to server
         * @param callId which call the message is sent about
         */
        void messageSent( bool success, const string& reason, string callId );
        
        void startCancel( string callId );
        void copyController( RpcController& dest, const RpcController& src );
        void copyMessage( Message& dest, const Message& src );
        bool isCallCompleted( const string& callId );
        void handleRequestTimeout( string callId );
        
    private:
        // <= 0, no request timeout, > 0, request timeout in milliseconds
        int timeoutMillis_;
        // map for waiting response: key is request callId, value is the parameters in the CallMethod())
        scoped_ptr< ThreadSafeMap< string, shared_ptr<ResponseParam> > > waitingResponses_;
        
        // timer management for request
        scoped_ptr< Timer<string> > timer_;
        
        //the received but not processed response messages
        scoped_ptr< Queue<string> > responses_;
        //the response process thread
        boost::thread_group responseProcThreads_;
    };

}//end name space pbrpcpp

#endif	/* BASERPCCHANNEL_HPP */

