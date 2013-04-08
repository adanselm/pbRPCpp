/* 
 * File:   UdpRpcServer.hpp
 * Author: Steven
 *
 * Created on April 2, 2013, 12:59 PM
 */

#ifndef UDPRPCSERVER_HPP
#define	UDPRPCSERVER_HPP

#include "BaseRpcServer.hpp"
#include <boost/asio.hpp>

using boost::asio::ip::udp;

namespace pbrpcpp {
    class UdpRpcServer: public BaseRpcServer {
    public:
        UdpRpcServer( const string& listenAddr, const string& listenPort);
        ~UdpRpcServer();
        void Run();
        void Shutdown();
    protected:
        virtual void sendResponse( int clientId, const string& msg );
    private:
        struct ClientData {
            ClientData( int clientId )
            :clientId_( clientId )
            {
                
            }
            int clientId_;
            udp::endpoint client_;
            char msgBuffer_[64*1024];
            
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
        void startRead( );
        void packetReceived( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        shared_ptr<ClientData> clientData  );
        void messageSent( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        string* buf );
    private:
        string listenAddr_;
        string listenPort_;
        int nextClientId_;
        char msgBuffer_[64*1024];
        boost::asio::io_service io_service_;
        udp::socket socket_;
        ClientDataMgr clientDataMgr_;
    };
}

#endif	/* UDPRPCSERVER_HPP */

