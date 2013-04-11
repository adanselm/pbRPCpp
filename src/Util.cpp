
/* 
 * File:   Util.cpp
 * Author: Steven
 *
 * Created on March 17, 2013, 11:15 PM
 */

#include "Util.hpp"
#include "RpcController.hpp"
#include <sstream>

using google::protobuf::DescriptorPool;
using google::protobuf::MessageFactory;

using std::istringstream;
using std::ostringstream;

namespace pbrpcpp {

    void Util::writeMethodDescriptor( const MethodDescriptor& method, ostream& out ) {
        writeString( method.full_name(), out );
    }

    const MethodDescriptor* Util::readMethodDescriptor( istream& in ) {
        return DescriptorPool::generated_pool()->FindMethodByName( readString( in ) );
    }

    void Util::writeMessage( const Message& msg, ostream& out ) {
        writeString( msg.GetDescriptor()->full_name(), out );
        msg.SerializeToOstream( &out );
    }


    void Util::readMessage( istream& in, Message*& msg ) {
        const Descriptor* reqDescriptor = DescriptorPool::generated_pool()->FindMessageTypeByName( readString( in ) );
        if( reqDescriptor ) {
                msg = MessageFactory::generated_factory()->GetPrototype( reqDescriptor )->New();
                if( msg ) {
                    msg->ParsePartialFromIstream( &in );
                } else {
                    throw runtime_error( "fail to read message");
                }
        } else {
            throw runtime_error( "fail to read Message");
        }


    }

    Message* Util::readMessage( istream& in ) {
        Message* msg = 0;

        readMessage( in, msg );

        return msg;
    }

    bool Util::equals( const Message& msg1, const Message& msg2 ) {
        ostringstream out_1;
        ostringstream out_2;
        
        writeMessage( msg1, out_1 );
        writeMessage( msg2, out_2 );
        
        return out_1.str() == out_2.str();
    }
    
    void Util::writeController( const RpcController& controller, ostream& out ) {
        controller.serializeTo( out );
    }

    void Util::readController( istream& in,  RpcController& controller ) {
        controller.parseFrom( in );
    }



    void Util::writeChar( char ch, ostream& out ) {
        out.put( ch );
    }

    char Util::readChar( const string& s, size_t& offset ) {
        if( s.length() < offset + sizeof( char )) {
            throw runtime_error("fail to read a char");
        }
        
        char ch = s[offset];        
        offset += sizeof( char );
        
        return ch;
    }
    
    char Util::readChar( istream & in ) {
        char ch = in.get();
        if( in.gcount() != sizeof( char ) ) {
            throw runtime_error("fail to read a char");
        }

        return ch;
    }

    void Util::writeInt( int i, ostream& out ) {
        char buf[4];
        buf[0] = (char)((i >> 24 )&0xff);
        buf[1] = (char)((i >> 16 )&0xff);
        buf[2] = (char)((i >> 8 )&0xff);
        buf[3] = (char)((i >> 0 )&0xff);

        out.write( buf, sizeof( buf ));
    }

    int Util::readInt( const string& s, size_t& offset ) {
        if( s.length() < offset + 4 ) {
            throw runtime_error( "fail to read int from string");
        }
        
        int i = ( ( s[offset] & 0xff ) << 24  ) | ( ( s[offset+1] & 0xff ) << 16  ) | ( ( s[offset+2] & 0xff ) << 8  ) | ( ( s[offset+3] & 0xff ) << 0  );
        offset += 4;
        
        return i;
    }
    int Util::readInt( istream& in ) {
        char buf[4];

        in.read( buf, sizeof( buf ));

        if( in.gcount() == 4 ) {
            return ( ( buf[0] & 0xff ) << 24  ) | ( ( buf[1] & 0xff ) << 16  ) | ( ( buf[2] & 0xff ) << 8  ) | ( ( buf[3] & 0xff ) << 0  );
        }
        throw runtime_error( "fail to read int from stream");
    }


    void Util::writeString( const string& s, ostream& out ) {


        writeInt( s.length(), out );
        if( s.length() > 0 ) {
                out.write( s.data(), s.length() );
        }
     }

    string Util::readString( istream& in ) {
        int n = readInt( in );
        if( n > 0 ) {
            char* buf = new char[ n ];

            in.read( buf, n );

            if( in.gcount() == n ) {
                string s( buf, n );
                delete []buf;
                return s;
            } else {
                throw runtime_error( "fail to read a string from input stream");
            }
        }
        return string();
    }

}//end name space pbrpcpp
    




