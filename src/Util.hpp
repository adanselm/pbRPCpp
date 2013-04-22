/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef UTIL_HPP
#define	UTIL_HPP

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include <ostream>
#include <istream>
#include <stdexcept>
#include <boost/smart_ptr.hpp>

using boost::shared_ptr;
using std::string;
using std::ostream;
using std::istream;
using std::runtime_error;
using google::protobuf::MethodDescriptor;
using google::protobuf::Message;
using google::protobuf::Descriptor;


namespace pbrpcpp {
    
    class RpcController;
    
    class Util {
    public:
        static void writeMethodDescriptor( const MethodDescriptor& method, ostream& out );

        static const MethodDescriptor* readMethodDescriptor( istream& in );

        static void writeMessage( const Message& msg, ostream& out );

        static void readMessage( istream& in, shared_ptr<Message>& msg );

        static shared_ptr<Message> readMessage( istream& in );
        /**
         * check if the <code>msg1</code> and <code>msg2</code> are exactly same content message
         * @param msg1 the first message
         * @param msg2 the second message
         * @return true if the msg1 equals to msg2
         */
        static bool equals( const Message& msg1, const Message& msg2 );

        static void writeController( const RpcController& controller, ostream& out );

        static void readController( istream& in,  RpcController& controller );

        static void writeChar( char ch, ostream& out );

        static char readChar(const string& s, size_t& offset );
        
        static char readChar( istream & in );

        static void writeInt( int i, ostream& out );

        static int readInt( const string& s, size_t& offset );
        static int readInt( istream& in );


        static void writeString( const string& s, ostream& out );        
        static string readString( istream& in ) ;

    };
}//end name space pbrpcpp


#endif	/* UTIL_HPP */

