/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "UdpRpcChannel.hpp"
#include "RpcMessage.hpp"
#include "IoServiceInitializer.hpp"
#include <sstream>

using std::ostringstream;

namespace pbrpcpp {
    
    UdpRpcChannel::UdpRpcChannel(const string& serverAddr, const string& serverPort)
    : stop_( false ),
    serverAddr_(serverAddr),
    serverPort_(serverPort),
    io_service_initializer_( new IoServiceInitializer() ),
    socket_( io_service_initializer_->get_io_service() ) {
        
        udp::resolver resolver(io_service_initializer_->get_io_service() );
        udp::resolver::query query(serverAddr_, serverPort_);
        boost::system::error_code error;

        udp::resolver::iterator iter = resolver.resolve(query, error);

        if (error || iter == udp::resolver::iterator()) {
            GOOGLE_LOG(FATAL) << "fail to resolve address " << serverAddr_ << ":" << serverPort_;
        } else {
            remoteEndpoint_ = *iter;

            socket_.open(remoteEndpoint_.protocol(), error);
            if( error ) {
                GOOGLE_LOG( ERROR ) << "fail to open the UDP socket";
            } else {
                socket_.set_option(udp::socket::reuse_address(true), error);
                socket_.set_option(udp::socket::send_buffer_size( RpcMessage::MAX_UDP_SIZE ), error);
                socket_.set_option(udp::socket::receive_buffer_size( RpcMessage::MAX_UDP_SIZE ), error);
                startRead();
            }
        }
    }

    UdpRpcChannel::~UdpRpcChannel() {
        close();
    }

    void UdpRpcChannel::close() {
        if (stop_) {
            return;
        }
        stop_ = true;

        boost::system::error_code error;

        socket_.cancel(error);
        socket_.close(error);
        io_service_initializer_->stop();
    }

    void UdpRpcChannel::sendMessage(const string& msg, boost::function< void (bool, const string&) > resultCb) {
        if( stop_ ) {
            return;
        }
                
        shared_ptr<string> s( new string( RpcMessage::serializeNetPacket( msg ) ) );

        GOOGLE_LOG( INFO ) << "start to send message to server with " << s->length() << " bytes";
        
        socket_.async_send_to(boost::asio::buffer(s->data(), s->length()),
                remoteEndpoint_,
                boost::bind(&UdpRpcChannel::handlePacketWrite, this, _1, _2, s, resultCb));

    }

    void UdpRpcChannel::startRead() {

        if( stop_ ) {
            return;
        }
        
        GOOGLE_LOG( INFO ) << "start to read from server";
        
        socket_.async_receive_from(boost::asio::buffer(msgBuffer_, sizeof ( msgBuffer_)), senderEndpoint_, boost::bind(&UdpRpcChannel::packetReceived, this, _1, _2));

    }

    void UdpRpcChannel::packetReceived(const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (stop_) {
            return;
        }

        if (senderEndpoint_ != remoteEndpoint_) {
            GOOGLE_LOG(ERROR) << "received a packet not from server, discard it";
            return;
        }
        string s(msgBuffer_, bytes_transferred);

        try {
            string resp_msg;
            
            while( RpcMessage::extractNetPacket( s, resp_msg ) ) {
                responseReceived( resp_msg );
            }
        } catch (...) {

        }
        startRead();

    }

    void UdpRpcChannel::handlePacketWrite(const boost::system::error_code& ec,
            std::size_t bytes_transferred,
            shared_ptr<string> buf,
            boost::function< void (bool, const string&) > resultCb) {

        if (ec) {
            GOOGLE_LOG(ERROR) << "fail to send packet to server";
            resultCb(false, "fail to send packet to server");
        } else {
            GOOGLE_LOG(INFO) << "succeed to send " << bytes_transferred << " bytes to server";
            resultCb(true, "success to send the packet to server");
        }
    }
}//end name space pbrpcpp
