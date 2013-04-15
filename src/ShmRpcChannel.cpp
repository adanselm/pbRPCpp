#include "ShmRpcChannel.hpp"
#include "ShmConnection.hpp"
#include "RpcMessage.hpp"
#include <sstream>

using std::ostringstream;

namespace pbrpcpp {

  ShmRpcChannel::ShmRpcChannel(const string& segmentName)
    : stop_( false ),
    inQueue_(new ShmConnection()),
    outQueue_(new ShmConnection())
  { 
    if( ! inQueue_->startConnect(segmentName + "-s2c", boost::bind( &ShmRpcChannel::messageReceived, this, _1))
        || ! outQueue_->startConnect(segmentName + "-c2s") )
    {
      GOOGLE_LOG(FATAL) << "fail to connect to segment " << segmentName;
    }
  }

  ShmRpcChannel::~ShmRpcChannel() {
    close();
  }

  void ShmRpcChannel::close() {
    if (stop_) {
      return;
    }
    stop_ = true;

    inQueue_->disconnect();
    outQueue_->disconnect();
  }

  void ShmRpcChannel::sendMessage(const string& msg, boost::function< void (bool, const string&) > resultCb) {
    if( stop_ ) {
      return;
    }

    shared_ptr<string> s( new string( RpcMessage::serializeNetPacket( msg ) ) );

    GOOGLE_LOG( INFO ) << "start to send message to server with " << s->length() << " bytes";

    const bool ret = outQueue_->sendMessage(*s);

    if( ! ret )
    {
      GOOGLE_LOG(ERROR) << "fail to send packet to server";
      resultCb(false, "fail to send packet to server");
    }
    else
    {
      GOOGLE_LOG(INFO) << "succeed to send message to server";
      resultCb(true, "succeed to send the packet to server");
    }
  }

  void ShmRpcChannel::messageReceived(const string& msg)
  {
    if (stop_) {
      return;
    }

    try {
      string temp(msg);
      string resp_msg;

      while( RpcMessage::extractNetPacket( temp, resp_msg ) ) {
        responseReceived( resp_msg );
      }
    } catch (...) {

    }
  }

}//end name space pbrpcpp
