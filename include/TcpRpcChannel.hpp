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
#include "RpcMessage.hpp"

using boost::asio::ip::tcp;
using boost::shared_ptr;
using boost::scoped_ptr;

namespace pbrpcpp {
    class IoServiceInitializer;

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
        volatile bool stop_;
        bool connectTried_;
        string serverAddr_;
        string serverPort_;
        
        char msgBuffer_[RpcMessage::TCP_MSG_BUFFER_SIZE];
        string receivedMsg_;
        scoped_ptr<IoServiceInitializer> io_service_initializer_;
        tcp::socket sock_;
    };
}//end name space pbrpcpp


#endif	/* TCPRPCCHANNEL_HPP */

