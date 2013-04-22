/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RPCMESSAGE_HPP
#define	RPCMESSAGE_HPP

#include <google/protobuf/service.h>
#include <ostream>
#include <istream>
#include <string>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

using std::string;
using std::ostream;
using std::istream;

using google::protobuf::MethodDescriptor;
using google::protobuf::Message;

namespace pbrpcpp {
    class RpcController;

    class RpcMessage {
    public:
        enum {
            UNDEFINE_MSG = 0,
            REQUEST_MSG = 1,
            RESPONSE_MSG = 2,
            CANCEL_MSG = 3
        };
        
        enum {
            MAX_UDP_SIZE = 64 * 1024, //max size of UDP message
            TCP_MSG_BUFFER_SIZE = 4096 //buffer size of message over TCP/IP
        };

        /**
         * serialize a request message to the output stream <code>out</code>
         * 
         * @param callId IN the unique callId to identify the request
         * @param method IN which method to call
         * @param request IN the request content
         * @param out OUT the serialization result output stream <code>out</code>
         */
        static void serializeRequest( const string& callId,
                    const MethodDescriptor& method,
                    const Message& request,
                    ostream& out );

        /**
         * serialize a response to a output stream <code>out</code>
         * 
         * @param callId IN the unique callId passed from the request to server
         * @param controller IN the RPC controller
         * @param response IN optional response message, this message must be present if the no error indicated in the controller
         * @param out OUT the serialization result output stream <code>out</code>
         */
        static void serializeResponse( const string& callId, 
                                            const RpcController& controller,    
                                            const Message* response,
                                            ostream& out );


        /**
         * serialize a cancel request to the output stream <code>out</code>
         * 
         * @param callId IN the unique request identifier
         * @param out OUT output stream to accept the serialization result
         */
        static void serializeCancel( const string& callId, ostream& out );

        /**
         * parse a request from a input stream <code>in</code> 
         * 
         * @param in IN the request input stream <code>in</code>
         * @param callId OUT user supplied buffer to accept the parsed callId
         * @param method OUT user supplied buffer to accept the parsed method
         * @param request OUT user supplied buffer to accept the parsed request message
         */
        static void parseRequestFrom( istream& in, string& callId, const MethodDescriptor*& method, shared_ptr<Message>& request );

        /**
         * parse a response from a input stream <code>in</code>
         * @param in IN the response input stream
         * @param callId OUT the user supplied buffer to accept the parsed request callId
         * @param controller OUT the user supplied buffer to accept the parsed controller
         * @param response OUT the user supplied buffer to accept the parsed response, this response maybe NULL if error information is in controller
         */
        static void parseResponseFrom( istream& in, string& callId, RpcController& controller, shared_ptr<Message>& response );

        /**
         * parse the cancel request from the input stream <code>in</code>
         * @param in the cancel request input stream <code>in</code>
         * @param callId OUT the user supplied buffer to accept the canceled request callId
         */
        static void parseCancelFrom( istream& in, string& callId );
        
        static string serializeNetPacket( const string& packet );
        
        /**
         * extract a Net packet serialized by serializeNetPacket() method
         * 
         * @param s IN/OUT the packet serialized result by serializeNetPacket() method
         * after a packet is extracted, the string <code>s</code> will be shorten
         * 
         * @param packet OUT the extracted packet
         * @return true if success to extract the packet from s
         */
        static bool extractNetPacket( string& s, string& packet );
    };

}//end name space pbrpcpp


#endif	/* RPCMESSAGE_HPP */

