/* 
 * File:   TcpRpcServer.hpp
 * Author: stou
 *
 * Created on March 31, 2013, 8:07 PM
 */

#ifndef TCPRPCSERVER_HPP
#define	TCPRPCSERVER_HPP

#include "BaseRpcServer.hpp"
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>

using boost::asio::ip::tcp;
using boost::shared_ptr;

namespace pbrpcpp {
    class TcpRpcServer: public BaseRpcServer {
    public:
        TcpRpcServer( const string& listenAddr, const string& listenPort);
        ~TcpRpcServer();
        void Run();
        void Shutdown();
    protected:
        virtual void sendResponse( int clientId, const string& msg );
    private:
        struct ClientData {
            ClientData( int clientId, tcp::socket* clientSock );
            ~ClientData() ;            
            bool extractMessage( string& msg ) ;
            
            int clientId_;
            tcp::socket* clientSock_;
            char msgBuffer_[4096];
            string receivedMsg_;
            
        };
        
        class ClientDataMgr {
        public:
            void addClient( shared_ptr<ClientData> clientData ) {
                boost::lock_guard<  boost::mutex > guard( mutex_ );
                
                clients_.insert( map< int,  shared_ptr<ClientData> >::value_type( clientData->clientId_, clientData ) );
            }
            shared_ptr<ClientData> removeClient( int clientId ) {
                boost::lock_guard<  boost::mutex > guard( mutex_ );
                
                map< int,  shared_ptr<ClientData> >::iterator iter = clients_.find( clientId );
                
                if( iter == clients_.end() ) {
                    return shared_ptr<ClientData>();
                }
                
                shared_ptr<ClientData> ret = iter->second;
                clients_.erase( iter );
                return ret;
            }
            shared_ptr<ClientData> getClient( int clientId ) {
                boost::lock_guard<  boost::mutex > guard( mutex_ );
                
                map< int,  shared_ptr<ClientData> >::iterator iter = clients_.find( clientId );
                
                return ( iter == clients_.end() ) ? shared_ptr<ClientData>(): iter->second;
            }
        private:
            boost::mutex mutex_;
            map< int,  shared_ptr<ClientData> > clients_;
        };
    private:
        void startAccept();
        void connAccepted( tcp::socket* clientSock, const boost::system::error_code& ec );
        void startRead( shared_ptr<ClientData> clientData );
        void clientDataReceived( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        shared_ptr<ClientData> clientData  );
        string extractRpcMessaeg( boost::asio::streambuf& buf );
        void messageSent( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        string* buf );
    private:
        string listenAddr_;
        string listenPort_;
        int nextClientId_;
        boost::asio::io_service io_service_;
        shared_ptr<tcp::acceptor> acceptor_;
        ClientDataMgr clientDataMgr_;
    };
}

#endif	/* TCPRPCSERVER_HPP */

