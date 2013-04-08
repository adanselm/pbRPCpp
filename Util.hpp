/* 
 * File:   Util.hpp
 * Author: Steven
 *
 * Created on March 17, 2013, 11:15 PM
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


using std::string;
using std::ostream;
using std::istream;
using std::runtime_error;
using google::protobuf::MethodDescriptor;
using google::protobuf::Message;
using google::protobuf::Descriptor;


namespace pbrpcpp {//begin namespace
    
    class RpcController;
    
    class Util {
    public:
        static void writeMethodDescriptor( const MethodDescriptor& method, ostream& out );

        static const MethodDescriptor* readMethodDescriptor( istream& in );

        static void writeMessage( const Message& msg, ostream& out );


        static void readMessage( istream& in, Message*& msg );

        static Message* readMessage( istream& in );

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
}//end name space


#endif	/* UTIL_HPP */

