#include "TcpRpcServer.hpp"
#include <boost/bind.hpp>
#include <sstream>

using std::ostringstream;

namespace pbrpcpp {
    TcpRpcServer::ClientData::ClientData( int clientId, tcp::socket* clientSock )
    : clientId_(clientId),
    clientSock_(clientSock) {

    }

    TcpRpcServer::ClientData::~ClientData() {
        delete clientSock_;
    }

    bool TcpRpcServer::ClientData::extractMessage(string& msg) {
        try {
            size_t pos = 0;

            char ch = Util::readChar(receivedMsg_, pos);
            int n = Util::readInt(receivedMsg_, pos);
            if (ch != 'R' || n < 0) {
                boost::system::error_code ec;

                clientSock_->close(ec);
                return false;
            } else if (pos + n <= receivedMsg_.length()) {
                msg = receivedMsg_.substr(pos, n);
                receivedMsg_.erase(0, n + pos);
                return true;
            }
        } catch (...) {
        }
        return false;
    }
            
    TcpRpcServer::TcpRpcServer( const string& listenAddr, const string& listenPort)
    :listenAddr_( listenAddr ),
    listenPort_( listenPort ),
    nextClientId_( 0 )
    {        
    }
    
    TcpRpcServer::~TcpRpcServer() {
        Shutdown();
    }
    
    void TcpRpcServer::Run() {
        boost::system::error_code ec;

        tcp::resolver resolver(io_service_ );
        tcp::resolver::query query( listenAddr_, listenPort_ );
        tcp::resolver::iterator iter = resolver.resolve(query, ec );
        
        if( ec ) {
            GOOGLE_LOG( FATAL ) << "fail to resolve listening address " << listenAddr_ << ":" << listenPort_;
        } else {
            GOOGLE_LOG( INFO ) << "start to accept TCP connection";

            acceptor_.reset( new tcp::acceptor( io_service_, *iter ));
            acceptor_->set_option( tcp::socket::reuse_address(true), ec );
            startAccept();

            io_service_.run( ec );
        }
    }
    
    void TcpRpcServer::Shutdown() {
        if( stop_ ) {
            return;
        }
        
        if( acceptor_ ) {
            boost::system::error_code ec;
            acceptor_->cancel( ec );
        }
        
        BaseRpcServer::Shutdown();
        
        io_service_.stop();
    }

    void TcpRpcServer::sendResponse( int clientId, const string& msg ) {
        shared_ptr< ClientData > clientData = clientDataMgr_.getClient( clientId );
        if( clientData ) {
            ostringstream out;
            
            Util::writeChar( 'R', out );
            Util::writeString( msg, out );
            
            string* s = new string( out.str() );

            GOOGLE_LOG( INFO ) << "send response to client with " << s->length() << " bytes";
            
            boost::asio::async_write( *(clientData->clientSock_),
                    boost::asio::buffer( s->data(), s->length() ),
                    boost::asio::transfer_all(),
                    boost::bind( &TcpRpcServer::messageSent, this, _1, _2, s ) );
        } else {
            GOOGLE_LOG( ERROR ) << "fail to send response because the client is already disconnected";
        }
    }
    
    void TcpRpcServer::startAccept() {
        tcp::socket* clientSock = new tcp::socket( io_service_ );
        
        acceptor_->async_accept( *clientSock, boost::bind( &TcpRpcServer::connAccepted, this, clientSock, _1 ));
    }
    
    void TcpRpcServer::connAccepted( tcp::socket* clientSock, const boost::system::error_code& ec ) {
        if( ec ) {
            GOOGLE_LOG( ERROR ) << "fail to accept connection from client";
            delete clientSock;
        } else {
            GOOGLE_LOG( INFO ) << "a client connection is accepted";
            shared_ptr< ClientData > clientData( new ClientData( nextClientId_++, clientSock ) );
            clientDataMgr_.addClient( clientData );
            startRead( clientData );
            startAccept();
        }
    }
    
    void TcpRpcServer::startRead( shared_ptr<ClientData> clientData ) {
        if( clientData ) {
            boost::asio::async_read( *(clientData->clientSock_), 
                    boost::asio::buffer( clientData->msgBuffer_, sizeof( clientData->msgBuffer_ ) ),
                    boost::asio::transfer_at_least(1),
                    boost::bind( &TcpRpcServer::clientDataReceived, this, _1, _2, clientData ) );
        }
    }
    
    void TcpRpcServer::clientDataReceived( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        shared_ptr<ClientData> clientData ) {
        if( ec ) {
            GOOGLE_LOG( ERROR ) << "fail to receive data from client";
            clientDataMgr_.removeClient( clientData->clientId_ );
            return;
        }
        
        try {
            GOOGLE_LOG( INFO ) << bytes_transferred << " bytes received from client";
            clientData->receivedMsg_.append( clientData->msgBuffer_, bytes_transferred );
            string msg;
            while( clientData->extractMessage( msg ) ) {
                GOOGLE_LOG(INFO) << "a message is received";
                messageReceived( clientData->clientId_, msg );
            }
            
            startRead( clientData );
        }catch( ... ) {
        }                
    }
    
    
    void TcpRpcServer::messageSent( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        string* buf ) {
        delete buf;
        if( ec ) {
            GOOGLE_LOG( ERROR ) << "fail to send message to client";
        } else {
            GOOGLE_LOG( INFO ) << "success to send " << bytes_transferred << " bytes message to client";
        }
    }
    
    
}