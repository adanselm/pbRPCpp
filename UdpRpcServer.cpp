#include "UdpRpcServer.hpp"
#include <boost/bind.hpp>
#include <sstream>

using std::ostringstream;

namespace pbrpcpp {

    UdpRpcServer::UdpRpcServer(const string& listenAddr, const string& listenPort)
    :listenAddr_(listenAddr),
    listenPort_(listenPort),
    nextClientId_(0),
    socket_(io_service_) {

    }

    UdpRpcServer::~UdpRpcServer() {
        Shutdown();
    }
    void UdpRpcServer::Run() {
        udp::resolver resolver(io_service_);
        udp::resolver::query query(listenAddr_, listenPort_);
        boost::system::error_code error;

        udp::resolver::iterator iter = resolver.resolve(query, error);

        if (error || iter == udp::resolver::iterator()) {
            GOOGLE_LOG(ERROR) << "fail to resolve address " << listenAddr_ << ":" << listenPort_;
            return;
        }

        udp::endpoint ep = *iter;

        socket_.open(ep.protocol(), error);
        socket_.set_option(udp::socket::reuse_address(true), error);
        if (error) {
            GOOGLE_LOG(ERROR) << "fail to open a UDP socket";
            return;
        }
        socket_.bind(ep, error);
        if (error) {
            GOOGLE_LOG(ERROR) << "fail to bind address to UDP socket";
            return;
        }

        GOOGLE_LOG( INFO ) << "success to bind";
        startRead();
        
        io_service_.run(error);
        
        GOOGLE_LOG( INFO ) << "exit the server";
    }

    void UdpRpcServer::Shutdown() {
        if (stop_) {
            return;
        }
        
        BaseRpcServer::Shutdown();
        io_service_.stop();
    }

    void UdpRpcServer::sendResponse(int clientId, const string& msg) {
        if (stop_) {
            GOOGLE_LOG(INFO) << "server is stopped, no message will be sent to client";
            return;
        }

        shared_ptr< ClientData > clientData = clientDataMgr_.removeClient(clientId);

        if (clientData) {
            ostringstream out;

            Util::writeChar('R', out);
            Util::writeString(msg, out);

            string *s = new string(out.str());

            socket_.async_send_to(boost::asio::buffer(s->data(), s->length()),
                    clientData->client_,
                    boost::bind(&UdpRpcServer::messageSent, this, _1, _2, s ) );
        }
    }

    void UdpRpcServer::startRead() {
        if (stop_) {
            GOOGLE_LOG(INFO) << "server is stopped, no message will be read from client";
            return;
        }

        GOOGLE_LOG(INFO) << "start to read message from client";
        shared_ptr< ClientData > clientData(new ClientData(++nextClientId_));
        socket_.async_receive_from(boost::asio::buffer(msgBuffer_, sizeof ( msgBuffer_)),
                clientData->client_,
                boost::bind(&UdpRpcServer::packetReceived, this, _1, _2, clientData) );
    }

    void UdpRpcServer::packetReceived(const boost::system::error_code& ec,
            std::size_t bytes_transferred,
            shared_ptr< ClientData > clientData) {
        if (ec) {
            GOOGLE_LOG(INFO) << "fail to read data from client";
            return;
        }
        
        startRead();
        
        try {
            GOOGLE_LOG(INFO) << "a message is read from client with " << bytes_transferred << " bytes";
            string s(msgBuffer_, bytes_transferred);
            size_t pos = 0;

            char ch = Util::readChar(s, pos);
            int n = Util::readInt(s, pos);

            if (ch != 'R' || n + pos != bytes_transferred) {
                GOOGLE_LOG(ERROR) << "invalid message received from client";
            } else {
                clientDataMgr_.addClient(clientData);
                messageReceived(clientData->clientId_, s.substr(pos));
            }
        } catch (...) {

        }

    }

    void UdpRpcServer::messageSent(const boost::system::error_code& ec,
            std::size_t bytes_transferred,
            string* buf) {
        delete buf;
        if (ec) {
            GOOGLE_LOG(ERROR) << "fail to send message to client";
        } 
    }

}