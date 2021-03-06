/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef SHMCONNECTION_HPP
#define SHMCONNECTION_HPP

#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>

using boost::scoped_ptr;

namespace pbrpcpp {
  
  class ShmConnection
  {
  public:
    ShmConnection();
    ~ShmConnection();
    
    bool startConnect(const std::string& segmentName,
                      boost::function< void (const std::string&) > receiveCb = 0);
    bool startCreate(const std::string& segmentName,
                     boost::function< void (const std::string&) > receiveCb = 0);
    void disconnect();
    bool isConnected() const;
    
    bool sendMessage( const std::string& msg );
    
  private:
    void startRead();
    bool connectToSegment(const std::string& segmentName);
    bool createSegment(const std::string& segmentName);
    void readLoop();
    bool readNextMessage();
    
  private:
    size_t mMaxNumMsg;
    size_t mMaxMsgSize;
    bool stop_;
    std::string segmentName_;
    
    std::string receivedMsg_;
    boost::mutex segmentMutex_;
    scoped_ptr<boost::interprocess::message_queue> queue_;
    boost::thread readThread_;
    boost::function< void (const std::string&) > receiveCb_;
  };
}//end name space pbrpcpp


#endif /* SHMCONNECTION_HPP */

