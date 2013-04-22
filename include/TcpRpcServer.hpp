/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TCPRPCSERVER_HPP
#define	TCPRPCSERVER_HPP

#include "BaseRpcServer.hpp"
#include "RpcMessage.hpp"
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>

using boost::asio::ip::tcp;
using boost::shared_ptr;
using boost::scoped_ptr;

namespace pbrpcpp {
    template <class T, class U> class ThreadSafeMap;

    class TcpRpcServer: public BaseRpcServer {
    public:
        TcpRpcServer( const string& listenAddr, const string& listenPort);
        ~TcpRpcServer();
        void Run();
        void Shutdown();
        bool getLocalEndpoint( tcp::endpoint& ec ) const;
        bool getLocalEndpoint( string& addr, string& port ) const;
    protected:
        virtual void sendResponse( int clientId, const string& msg );
    private:
        struct ClientData {
            ClientData( int clientId, boost::asio::io_service& io_service );
            ~ClientData() ;            
            bool extractMessage( string& msg ) ;
            
            int clientId_;
            tcp::socket clientSock_;
            char msgBuffer_[RpcMessage::TCP_MSG_BUFFER_SIZE];
            string receivedMsg_;
        };
        
        
    private:
        void startAccept();
        void connAccepted( shared_ptr<ClientData> clientData, const boost::system::error_code& ec );
        void startRead( shared_ptr<ClientData> clientData );
        void clientDataReceived( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        shared_ptr<ClientData> clientData  );
        string extractRpcMessaeg( boost::asio::streambuf& buf );
        void messageSent( const boost::system::error_code& ec, 
                        std::size_t bytes_transferred, 
                        shared_ptr<string> buf );
    private:
        //the server listening address
        string listenAddr_;
        //the listening port
        string listenPort_;
        //the next client identifier
        int nextClientId_;
        volatile bool io_service_stopped_;
        boost::asio::io_service io_service_;
        shared_ptr<tcp::acceptor> acceptor_;
        scoped_ptr< ThreadSafeMap< int, shared_ptr<ClientData> > > clientDataMgr_;
    };
}//end name space pbrpcpp

#endif	/* TCPRPCSERVER_HPP */

