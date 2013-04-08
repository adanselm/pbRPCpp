/* 
 * File:   RpcMessage.cpp
 * Author: Steven
 *
 * Created on March 17, 2013, 11:15 PM
 */

#include "RpcMessage.hpp"

namespace pbrpcpp {    
    void RpcMessage::serializeRequest( const string& callId,
                const MethodDescriptor& method,
                const Message& request,
                ostream& out ) {
        Util::writeInt( REQUEST_MSG, out );
        Util::writeString( callId, out );
        Util::writeMethodDescriptor( method, out );
        Util::writeMessage( request, out );
    }

    void RpcMessage::serializeResponse( const string& callId, 
                                        const RpcController& controller,    
                                        const Message* response,
                                        ostream& out ) {

        Util::writeInt( RESPONSE_MSG, out );
        Util::writeString( callId, out );
        Util::writeController( controller, out );
        if( response ) {
            out.put( 'Y');
            Util::writeMessage( *response, out );
        } else {
            out.put( 'N');
        }
    }


    void RpcMessage::serializeCancel( const string& callId, ostream& out ) {
        Util::writeInt( CANCEL_MSG, out );
        Util::writeString( callId, out );
    }


    void RpcMessage::parseRequestFrom( istream& in, string& callId, const MethodDescriptor*& method, Message*& request ) {
        callId = Util::readString( in );
        method = Util::readMethodDescriptor( in );
        request = Util::readMessage( in );
    }

    void RpcMessage::parseResponseFrom( istream& in, string& callId, RpcController& controller, Message*& response ) {
        callId = Util::readString( in );
        Util::readController( in, controller );
        char c = in.get();
        if( c == 'Y') {
            response = Util::readMessage( in );
        } else {
            response = 0;
        }
    }

    void RpcMessage::parseCancelFrom( istream& in, string& callId ) {
        callId = Util::readString( in );
    }

}



