/* 
 * File:   UdpRpcChannel.hpp
 * Author: Steven
 *
 * Created on April 2, 2013, 1:00 PM
 */

#ifndef UDPRPCCHANNEL_HPP
#define	UDPRPCCHANNEL_HPP

#include "BaseRpcChannel.hpp"
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "RpcMessage.hpp"

using boost::asio::ip::udp;
using boost::shared_ptr;
using boost::scoped_ptr;

namespace pbrpcpp {
    class IoServiceInitializer;
    
    class UdpRpcChannel: public BaseRpcChannel {
    public:
        UdpRpcChannel( const string& serverAddr, const string& serverPort );
        ~UdpRpcChannel();
        
        void close();
    protected:
        void sendMessage( const string& msg,  boost::function< void (bool, const string&) > resultCb );
    private:
        void startRead();
        void packetReceived(const boost::system::error_code& ec, 
                        std::size_t bytes_transferred );
        void handlePacketWrite( const boost::system::error_code& ec,
                       std::size_t bytes_transferred,
                       string* buf,
                       boost::function< void (bool, const string&) > resultCb );
    private:
        volatile bool stop_;
        string serverAddr_;
        string serverPort_;
        char msgBuffer_[RpcMessage::MAX_UDP_SIZE];
        udp::endpoint remoteEndpoint_;
        udp::endpoint senderEndpoint_;        
        scoped_ptr<IoServiceInitializer> io_service_initializer_;
        udp::socket socket_;
    };
}//end name space pbrpcpp

#endif	/* UDPRPCCHANNEL_HPP */

