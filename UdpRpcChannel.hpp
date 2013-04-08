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
#include "IoServiceInitializer.hpp"

using boost::asio::ip::udp;
using boost::shared_ptr;

namespace pbrpcpp {
    
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
        string serverAddr_;
        string serverPort_;
        char msgBuffer_[64*1024];
        udp::endpoint remoteEndpoint_;
        udp::endpoint senderEndpoint_;        
        IoServiceInitializer io_service_initializer_;
        udp::socket socket_;
    };
}

#endif	/* UDPRPCCHANNEL_HPP */

