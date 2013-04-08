/* 
 * File:   TcpRpcChannel.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 8:06 PM
 */

#ifndef TCPRPCCHANNEL_HPP
#define	TCPRPCCHANNEL_HPP

#include "BaseRpcChannel.hpp"
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "IoServiceInitializer.hpp"

using boost::asio::ip::tcp;
using boost::shared_ptr;

namespace pbrpcpp {
    
    class TcpRpcChannel: public BaseRpcChannel {
    public:
        TcpRpcChannel( const string& serverAddr, const string& serverPort );
        ~TcpRpcChannel();
        
        void close();
        bool isConnected() const {
            return sock_.is_open();
        }
    protected:
        void sendMessage( const string& msg,  boost::function< void (bool, const string&) > resultCb );
    private:
        void startConnect();
        void doConnect( tcp::resolver::iterator iter );        
        void serverConnected( const boost::system::error_code& ec, tcp::resolver::iterator iter );
        void startRead();
        void dataReceived(const boost::system::error_code& ec, 
                        std::size_t bytes_transferred );
        void handleDataWrite( const boost::system::error_code& ec,
                       std::size_t bytes_transferred,
                       string* buf,
                       boost::function< void (bool, const string&) > resultCb );
        bool extractMessage( string& msg );
    private:
        bool connectTried_;
        string serverAddr_;
        string serverPort_;
        char msgBuffer_[4096];
        string receivedMsg_;
        IoServiceInitializer io_service_initializer_;
        tcp::socket sock_;
    };
}


#endif	/* TCPRPCCHANNEL_HPP */

