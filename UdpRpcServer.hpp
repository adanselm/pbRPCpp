/* 
 * File:   UdpRpcServer.hpp
 * Author: Steven
 *
 * Created on April 2, 2013, 12:59 PM
 */

#ifndef UDPRPCSERVER_HPP
#define	UDPRPCSERVER_HPP

#include "BaseRpcServer.hpp"
#include "ThreadSafeMap.hpp"
#include "RpcMessage.hpp"
#include <list>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using boost::asio::ip::udp;
using boost::ptr_map;
using std::list;

namespace pbrpcpp {
    class UdpRpcServer: public BaseRpcServer {
    public:
        UdpRpcServer( const string& listenAddr, const string& listenPort);
        ~UdpRpcServer();
        void Run();
        void Shutdown();
        bool getLocalEndpoint( udp::endpoint& ec ) const;
        bool getLocalEndpoint( string& addr, string& port ) const;
    protected:
        virtual void sendResponse( int clientId, const string& msg );
    private:
        struct ClientData {
            /**
             * 
             * @param clientId client identifier
             * @param timeout in seconds
             * @param clientEndpoint client network address
             */
            ClientData( int clientId, int timeout, const udp::endpoint& clientEndpoint );
            
            bool isTimeout() const;
            
            void setTimeout( int timeout ) ;
            
            int clientId_;
            boost::posix_time::ptime timeoutTime_;
            udp::endpoint clientEndpoint_;
        };
        
        class ClientIdAllocator {
            
        public:
            ClientIdAllocator();            
            int allocClientId( const udp::endpoint& ep ) ;            
            bool getClientEndpoint( int clientId, udp::endpoint& ep ) const;
        private:
            void removeTimeoutClients();
        private:
            enum {
                CLIENT_TIMEOUT_SECONDS = 24*60*60 //client timeout is 24 hours
            };
            mutable boost::mutex mutex_;
            int nextClientId_;
            ptr_map< udp::endpoint, ClientData > clientIds_;
            map< int, udp::endpoint > clientEndpoints_;
        };
        
                        
    private:
        void startRead( );
        void packetReceived( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        shared_ptr< udp::endpoint > ep  );
        void messageSent( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        string* buf );
    private:
        string listenAddr_;
        string listenPort_;
        int nextClientId_;
        // buffer to receive the client message
        char msgBuffer_[RpcMessage::MAX_UDP_SIZE];
        bool io_service_stopped_;
        boost::asio::io_service io_service_;
        //socket to accept the client request
        udp::socket socket_;
        ClientIdAllocator clientIdAllocator_;
        
    };
}//end name space pbrpcpp

#endif	/* UDPRPCSERVER_HPP */

