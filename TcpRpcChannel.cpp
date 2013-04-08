/* 
 * File:   TcpRpcChannel.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 8:06 PM
 */

#include "TcpRpcChannel.hpp"
#include <boost/asio/basic_socket.hpp>
#include <sstream>
#include "BaseRpcChannel.hpp"
#include "Util.hpp"


using std::ostringstream;

namespace pbrpcpp {    
    TcpRpcChannel::TcpRpcChannel( const string& serverAddr, const string& serverPort )
    :connectTried_( false ),
     serverAddr_( serverAddr ),
    serverPort_( serverPort ),
    sock_( io_service_initializer_.get_io_service()  )
    {
        startConnect();
        while( !connectTried_ );
    }
    
    TcpRpcChannel::~TcpRpcChannel() {
        close();
    }
    
    void TcpRpcChannel::close() {
        if( stop_ ) {
            return;
        }
        
        stop_ = true;
        //close the the connection
        boost::system::error_code ec;
        sock_.cancel( ec );
        sock_.close( ec );
        io_service_initializer_.stop();
    }
    
    void TcpRpcChannel::startConnect() {
        //if already stopped
        if( stop_ ) {
            return;
        }
        
        
        boost::system::error_code ec;
        //close the socket at first 
        sock_.close( ec );
        
        //start to resolve the address
        tcp::resolver resolver( io_service_initializer_.get_io_service() );
        tcp::resolver::query query( serverAddr_, serverPort_ );
        tcp::resolver::iterator iter = resolver.resolve(query, ec  );
        
        if( ec ) {
            GOOGLE_LOG( ERROR ) << "fail a resolve server address " << serverAddr_ << ":" << serverPort_;
        } else {
            receivedMsg_.clear();
            doConnect( iter );
        }
    }
    
    void TcpRpcChannel::doConnect(  tcp::resolver::iterator iter ) {
        
        //try to connect to server
        if( iter != tcp::resolver::iterator() ) {            
            sock_.async_connect( *iter, boost::bind( &TcpRpcChannel::serverConnected, this, _1, iter ) );
        } else {
            connectTried_ = true;
            startConnect();
        }
    }
    
    void TcpRpcChannel::serverConnected( const boost::system::error_code& ec, tcp::resolver::iterator iter ) {
        if( ec ) {
            if( iter == tcp::resolver::iterator() ) {
                connectTried_ = true;
                startConnect();                
            } else {
                doConnect( ++iter );
            }
        } else {
            //the connection to server is established
            connectTried_ = true;
            GOOGLE_LOG( INFO ) << "connect to server " << serverAddr_ << ":" << serverPort_ << " successfully";
            boost::system::error_code error;            
            sock_.set_option(tcp::socket::reuse_address(true), error );
            startRead();
        }
    }
    
    void TcpRpcChannel::startRead() {
        if( stop_ ) {
            return;
        }
        
        boost::asio::async_read( sock_,
                        boost::asio::buffer( msgBuffer_, sizeof( msgBuffer_ )), 
                        boost::asio::transfer_at_least(1),
                        boost::bind( &TcpRpcChannel::dataReceived, this, _1, _2 ) );
                
    }
        
    
    void TcpRpcChannel::sendMessage( const string& msg, boost::function< void (bool, const string&) > resultCb ) {
        if( stop_ ) {
            return;
        }
        
        ostringstream out;
        
        Util::writeChar( 'R', out );
        Util::writeString( msg, out);
        string* s = new string( out.str() );
                        
        if( sock_.is_open() ) {
            GOOGLE_LOG( INFO ) << "send a message to server with " << s->length() << " bytes";
            boost::asio::async_write( sock_, 
                    boost::asio::buffer( s->data(), s->length() ), 
                    boost::asio::transfer_all(),
                    boost::bind( &TcpRpcChannel::handleDataWrite, this, _1, _2, s, resultCb ) );
        } else {
            GOOGLE_LOG( ERROR ) << "fail to send to client because not connect to server";
            resultCb( false, "server is not connected");
        }
        
    }
    
    void TcpRpcChannel::dataReceived( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred ) {
        if( ec ) {
            GOOGLE_LOG( ERROR ) << "fail to receive data from server";
            startConnect();
            return;
        }
        
        GOOGLE_LOG( INFO ) << "received " << bytes_transferred << " bytes from server";
        receivedMsg_.append( msgBuffer_, bytes_transferred );        
        
        startRead();
        
        string msg;
        
        while( extractMessage( msg ) ) {
            responseReceived( msg );
        }
        
    }
    void TcpRpcChannel::handleDataWrite( const boost::system::error_code& ec,
                       std::size_t bytes_transferred,
                       string* buf,
                       boost::function< void (bool, const string&) > resultCb ) {        
        delete buf;
        
        if( ec ) {
            GOOGLE_LOG( ERROR ) << "fail to send message to server";
            resultCb( false, "fail to connect to server");
        } else {
            GOOGLE_LOG( INFO ) << "success to send " << bytes_transferred << " bytes message to server";
        }
        
    }
    
    bool TcpRpcChannel::extractMessage( string& msg ) {
        try {
            size_t pos = 0;

            char ch = Util::readChar( receivedMsg_, pos );
            int n = Util::readInt( receivedMsg_, pos );
            if( ch != 'R' || n < 0 ) {
                boost::system::error_code ec;                
                sock_.cancel( ec );
                sock_.shutdown( tcp::socket::shutdown_both, ec );
                sock_.close( ec );
                return false;
            } else if( n + pos <= receivedMsg_.length() ) {
                msg = receivedMsg_.substr( pos, n );
                receivedMsg_.erase( 0, pos + n );
                return true;
            }
        }catch( ... ) {
            
        }
        
        return false;
    }
    
}


