/* 
 * File:   RpcMessage.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 5:50 PM
 */

#ifndef RPCMESSAGE_HPP
#define	RPCMESSAGE_HPP

#include "RpcController.hpp"
#include <google/protobuf/service.h>
#include <ostream>
#include <string>

using std::string;
using std::ostream;

using google::protobuf::MethodDescriptor;
using google::protobuf::Message;

namespace pbrpcpp {
    class RpcMessage {
    public:
        enum {
            UNDEFINE_MSG = 0,
            REQUEST_MSG = 1,
            RESPONSE_MSG = 2,
            CANCEL_MSG = 3
        };

        static void serializeRequest( const string& callId,
                    const MethodDescriptor& method,
                    const Message& request,
                    ostream& out );

        static void serializeResponse( const string& callId, 
                                            const RpcController& controller,    
                                            const Message* response,
                                            ostream& out );


        static void serializeCancel( const string& callId, ostream& out );


        static void parseRequestFrom( istream& in, string& callId, const MethodDescriptor*& method, Message*& request );

        static void parseResponseFrom( istream& in, string& callId, RpcController& controller, Message*& response );

        static void parseCancelFrom( istream& in, string& callId );


    };

}


#endif	/* RPCMESSAGE_HPP */

