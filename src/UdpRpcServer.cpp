#include "UdpRpcServer.hpp"
#include "Util.hpp"
#include <boost/bind.hpp>
#include <sstream>

using std::ostringstream;

namespace pbrpcpp {

    //ClientData implementation

    UdpRpcServer::ClientData::ClientData(int clientId, int timeout, const udp::endpoint& clientEndpoint)
    : clientId_(clientId),
    timeoutTime_(boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(timeout)),
    clientEndpoint_(clientEndpoint) {

    }

    bool UdpRpcServer::ClientData::isTimeout() const {
        return timeoutTime_ <= boost::posix_time::microsec_clock::universal_time();
    }

    void UdpRpcServer::ClientData::setTimeout(int timeout) {
        timeoutTime_ = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(timeout);
    }


    //
    // ClientIdAllocator implementation
    //

    UdpRpcServer::ClientIdAllocator::ClientIdAllocator()
    : nextClientId_(0) {

    }

    int UdpRpcServer::ClientIdAllocator::allocClientId(const udp::endpoint& ep) {
        boost::lock_guard< boost::mutex > guard(mutex_);

        ptr_map< udp::endpoint, ClientData >::iterator iter = clientIds_.find(ep);
        int clientId = 0;

        //add new client info is not found
        //update the timeout info is the client is found
        if (iter == clientIds_.end()) {
            clientId = nextClientId_++;
            clientIds_.insert(ep, std::auto_ptr<ClientData>(new ClientData(clientId, CLIENT_TIMEOUT_SECONDS, ep)));
            clientEndpoints_[ clientId ] = ep;
        } else {
            clientId = iter->second->clientId_;
            iter->second->setTimeout(CLIENT_TIMEOUT_SECONDS);
        }

        removeTimeoutClients();

        return clientId;
    }

    bool UdpRpcServer::ClientIdAllocator::getClientEndpoint(int clientId, udp::endpoint& ep) const {
        boost::lock_guard< boost::mutex > guard(mutex_);

        map< int, udp::endpoint >::const_iterator iter = clientEndpoints_.find(clientId);

        if (iter == clientEndpoints_.end()) {
            return false;
        }

        ep = iter->second;

        return true;

    }

    void UdpRpcServer::ClientIdAllocator::removeTimeoutClients() {
        list< udp::endpoint > timeoutClients;

        //find all the timeout client and put to the timeoutClients list
        for (ptr_map< udp::endpoint, ClientData >::iterator iter = clientIds_.begin(); iter != clientIds_.end(); iter++) {
            if (iter->second->isTimeout()) {
                timeoutClients.push_back(iter->first);
            }
        }

        //remove all the timeout client information
        for (list< udp::endpoint >::const_iterator iter_2 = timeoutClients.begin(); iter_2 != timeoutClients.end(); iter_2++) {
            ptr_map< udp::endpoint, ClientData >::iterator iter_3 = clientIds_.find(*iter_2);
            BOOST_VERIFY(iter_3 != clientIds_.end());
            clientEndpoints_.erase(iter_3->second->clientId_);
            clientIds_.erase(iter_3);
        }
    }

    //
    // class UdpRpcServer implementation
    //

    UdpRpcServer::UdpRpcServer(const string& listenAddr, const string& listenPort)
    : listenAddr_(listenAddr),
    listenPort_(listenPort),
    nextClientId_(0),
    io_service_stopped_(true),
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
        socket_.set_option(udp::socket::send_buffer_size( RpcMessage::MAX_UDP_SIZE ), error);
        socket_.set_option(udp::socket::receive_buffer_size( RpcMessage::MAX_UDP_SIZE ), error);
        if (error) {
            GOOGLE_LOG(ERROR) << "fail to open a UDP socket";
            return;
        }
        socket_.bind(ep, error);
        if (error) {
            GOOGLE_LOG(ERROR) << "fail to bind address to UDP socket";
            return;
        }

        startRead();

        io_service_stopped_ = false;

        io_service_.run(error);

        io_service_stopped_ = true;

        GOOGLE_LOG(INFO) << "exit the UDP server";
    }

    void UdpRpcServer::Shutdown() {

        if (!io_service_stopped_) {
            while (getProcessingRequests() > 0) {
                boost::this_thread::yield();
            }

            io_service_.stop();

            while (!io_service_stopped_);
        }
    }

    bool UdpRpcServer::getLocalEndpoint(udp::endpoint& ep) const {
        if( io_service_stopped_ ) {
            return false;
        }
        
        boost::system::error_code error;

        ep = socket_.local_endpoint(error);

        return !error;
    }

    bool UdpRpcServer::getLocalEndpoint(string& addr, string& port) const {
        udp::endpoint ep;

        if (getLocalEndpoint(ep)) {
            addr = ep.address().to_string();
            ostringstream out;

            out << ep.port();
            port = out.str();
            return true;
        }
        return false;
    }

    void UdpRpcServer::sendResponse(int clientId, const string& msg) {
        if (stop_) {
            GOOGLE_LOG(INFO) << "server is stopped, no message will be sent to client";
            return;
        }

        udp::endpoint ep;
        if (clientIdAllocator_.getClientEndpoint(clientId, ep)) {
            shared_ptr<string> s( new string(RpcMessage::serializeNetPacket(msg)) );

            GOOGLE_LOG(INFO) << "start to send " << s->length() << " bytes to server";

            socket_.async_send_to(boost::asio::buffer(s->data(), s->length()),
                    ep,
                    boost::bind(&UdpRpcServer::messageSent, this, _1, _2, s));
        } else {
            GOOGLE_LOG(ERROR) << "fail to send response because no client info is found";
        }
    }

    void UdpRpcServer::startRead() {
        if (stop_) {
            GOOGLE_LOG(INFO) << "server is stopped, no message will be read from client";
            return;
        }

        GOOGLE_LOG(INFO) << "start to read message from client";
        shared_ptr< udp::endpoint > ep(new udp::endpoint());
        socket_.async_receive_from(boost::asio::buffer(msgBuffer_, sizeof ( msgBuffer_)),
                *ep,
                boost::bind(&UdpRpcServer::packetReceived, this, _1, _2, ep));
    }

    void UdpRpcServer::packetReceived(const boost::system::error_code& ec,
            std::size_t bytes_transferred,
            shared_ptr< udp::endpoint > ep) {
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
                messageReceived(clientIdAllocator_.allocClientId(*ep), s.substr(pos));
            }
        } catch (const std::exception& ex) {
            GOOGLE_LOG(ERROR) << "catch exception:" << ex.what();
        } catch (...) {
            GOOGLE_LOG(ERROR) << "catch unknown exception";
        }

    }

    void UdpRpcServer::messageSent(const boost::system::error_code& ec,
            std::size_t bytes_transferred,
            shared_ptr<string> buf) {
        if (ec) {
            GOOGLE_LOG(ERROR) << "fail to send message to client";
        } else {
            GOOGLE_LOG(INFO) << "success to send " << bytes_transferred << " bytes to server";
        }
    }

}//end name space pbrpcpp
