/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "ShmRpcServer.hpp"
#include "ShmConnection.hpp"
#include "RpcMessage.hpp"
#include "Util.hpp"
#include <boost/bind.hpp>
#include <sstream>

using std::ostringstream;

namespace pbrpcpp {
    //
    // class ShmRpcServer implementation
    //

    ShmRpcServer::ShmRpcServer(const string& segmentName)
    : segmentName_(segmentName),
    inQueue_(new ShmConnection()),
    outQueue_(new ShmConnection())
    {
    }

    ShmRpcServer::~ShmRpcServer() {
        Shutdown();
    }

    void ShmRpcServer::Run() {
      if( ! inQueue_->startCreate(segmentName_ + "-c2s", boost::bind( &ShmRpcServer::messageReceived, this, _1))
          || ! outQueue_->startCreate(segmentName_ + "-s2c") )
      {
        GOOGLE_LOG(FATAL) << "fail to connect to segment " << segmentName_;
      }
    }

    void ShmRpcServer::Shutdown()
    {
      inQueue_->disconnect();
      outQueue_->disconnect();
    }

    void ShmRpcServer::sendResponse(int /*clientId*/, const string& msg)
    {
      if (stop_) {
        GOOGLE_LOG(INFO) << "server is stopped, no message will be sent to client";
        return;
      }

      if( ! outQueue_->isConnected() )
      {
        GOOGLE_LOG(FATAL) << "Output segment is not connected anymore";
        return;
      }

      shared_ptr<string> s( new string(RpcMessage::serializeNetPacket(msg)) );
      GOOGLE_LOG(INFO) << "start to send " << s->length() << " bytes back to client";

      if( outQueue_->sendMessage(*s) )
        GOOGLE_LOG(INFO) << "success to send response message back to client.";
      else
        GOOGLE_LOG(ERROR) << "fail to send response message back to client";

    }

    void ShmRpcServer::messageReceived(const string& msg)
    {
      try {
        GOOGLE_LOG(INFO) << "a message is read from client with " << msg.length() << " bytes";
        size_t pos = 0;

        char ch = Util::readChar(msg, pos);
        int n = Util::readInt(msg, pos);

        if (ch != 'R' || n + pos != msg.length()) {
          GOOGLE_LOG(ERROR) << "invalid message received from client";
        } else {
          BaseRpcServer::messageReceived(1, msg.substr(pos));
        }
      } catch (const std::exception& ex) {
        GOOGLE_LOG(ERROR) << "caught exception:" << ex.what();
      } catch (...) {
        GOOGLE_LOG(ERROR) << "caught unknown exception";
      }
    }

}//end name space pbrpcpp
