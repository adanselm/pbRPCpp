/*
 * File:   ShmConnection.cpp
 * Author: Adrien
 *
 * Created on April 13, 2013, 8:06 PM
 */

#include "ShmConnection.hpp"
/**
 * Temporary workaround needed to compile in 32bit.
 * See: https://svn.boost.org/trac/boost/ticket/6147
 */
namespace boost {
  namespace interprocess {
    namespace ipcdetail {
      
      //Rounds "orig_size" by excess to round_to bytes
      template<class SizeType, class ST2>
      inline SizeType get_rounded_size(SizeType orig_size, ST2 round_to)
      {
        return ((orig_size-1)/round_to+1)*round_to;
      }
    }
  }
}
/**********/
#include "boost/interprocess/ipc/message_queue.hpp"
#include "boost/scoped_ptr.hpp"
#include <sstream>

#ifndef SHM_MAX_MSG_SIZE
#define SHM_MAX_MSG_SIZE 32767
#endif
#ifndef SHM_MAX_MSG_COUNT
#define SHM_MAX_MSG_COUNT 170
#endif

using std::ostringstream;
using boost::interprocess::scoped_lock;
using boost::scoped_ptr;

namespace pbrpcpp {
  
  ShmConnection::ShmConnection()
  :
  mMaxNumMsg(SHM_MAX_MSG_COUNT),
  mMaxMsgSize(SHM_MAX_MSG_SIZE), //TODO TEMP VALUE
  stop_(false)
  {
  }
  
  ShmConnection::~ShmConnection()
  {
    disconnect();
  }
  
  void ShmConnection::disconnect()
  {
    if (stop_) {
      return;
    }
    
    stop_ = true;
    readThread_.join();
    if(queue_ != 0)
    {
      std::size_t aSize = queue_->get_num_msg();
//      GOOGLE_LOG(INFO) << "Disconnect called. Number of messages in queue: " << ((int)aSize);
      const scoped_lock<boost::mutex> sl(segmentMutex_);
      queue_.reset(0);
      boost::interprocess::message_queue::remove(segmentName_.c_str());
    }
  }
  
  bool ShmConnection::isConnected() const
  {
    //    const scoped_lock<boost::mutex> sl(segmentMutex_);
    return queue_ != 0;
  }
  
  bool ShmConnection::connectToSegment(const std::string& segmentName)
  {
    disconnect();
    stop_ = false;
    
    using namespace boost::interprocess;
    try
    {
      boost::scoped_ptr<message_queue> newQ(new message_queue(open_only,
                                                              segmentName.c_str()));
      {
        const scoped_lock<boost::mutex> sl(segmentMutex_);
        segmentName_ = segmentName;
        queue_.swap(newQ);
        return true;
      }
    }
    catch (boost::interprocess::interprocess_exception &ex)
    {
//      GOOGLE_LOG(ERROR) << "failed to connect to shared mem segment. " << ex.what();
      return false;
    }
  }
  
  bool ShmConnection::createSegment(const std::string& segmentName)
  {
//    LOG_DBG << "createSegment";
    disconnect();
    stop_ = false;
    
    using namespace boost::interprocess;
    try
    {
      boost::scoped_ptr<message_queue> newQ(new message_queue(create_only,
                                                              segmentName.c_str(),
                                                              mMaxNumMsg,
                                                              mMaxMsgSize));
      {
        const scoped_lock<boost::mutex> sl(segmentMutex_);
        segmentName_ = segmentName;
        queue_.swap(newQ);
        return true;
      }
    }
    catch (boost::interprocess::interprocess_exception &ex)
    {
//      LOG_ERR << ex.what();
      return false;
    }
  }
  
  void ShmConnection::startConnect(const std::string& segmentName)
  {
    //if already stopped
    if (stop_) {
      return;
    }
    
    bool ret = connectToSegment(segmentName);
    
    if(ret)
    {
//      receivedMsg_.clear();
//      receiveCb_ = receiveCb;
//      startRead();
    }
    else
    {
//      GOOGLE_LOG(ERROR) << "fail a resolve server address " << segmentName_;
    }
  }
  
  void ShmConnection::startCreate(const std::string& segmentName,
                                  boost::function< void (const std::string&) > receiveCb)
  {
    //if already stopped
    if (stop_) {
      return;
    }
    
    bool ret = createSegment(segmentName);
    
    if(ret)
    {
      receivedMsg_.clear();
      receiveCb_ = receiveCb;
      startRead();
    }
    else
    {
      //      GOOGLE_LOG(ERROR) << "fail a resolve server address " << segmentName_;
    }
  }
  
  void ShmConnection::startRead()
  {
    if (stop_) {
      return;
    }
    
    readThread_ = boost::thread(&ShmConnection::readLoop, this);    
  }
  
  void ShmConnection::readLoop()
  {
    while(!stop_ && queue_ != 0)
    {
      readNextMessage();
    }
  }
  
  bool ShmConnection::readNextMessage()
  {
    receivedMsg_.resize(mMaxMsgSize);
    boost::interprocess::message_queue::size_type aRecvdSize = 0;
    unsigned int aPriority = 0;
    bool aSuccess = false;
    
    try
    {
      while( !aSuccess )
      {
        if( stop_ )
          return false;
        
        // non-blocking dequeue
        aSuccess = queue_->try_receive(&receivedMsg_[0], receivedMsg_.size(),
                                       aRecvdSize, aPriority);
      }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
//      LOG_ERR << ex.what();
      return false;
    }
    
    // Timed out OR Received 0 bytes => exit
    if(!aSuccess || !aRecvdSize)
      return false;

    // Header check
    boost::uint32_t* messageHeader = reinterpret_cast<boost::uint32_t*>(&receivedMsg_[0]);
    if(messageHeader)
    {
      const boost::uint32_t bytesInMessage = messageHeader[0];
      if( bytesInMessage > 0
         && bytesInMessage < mMaxMsgSize
         && bytesInMessage == (boost::uint32_t)(aRecvdSize - sizeof(boost::uint32_t)) )
      {
        const std::string message = receivedMsg_.substr(sizeof(bytesInMessage)).c_str();
        receiveCb_(message);
        return true;
      }
    }
    
    return false;
  }

  
  bool ShmConnection::sendMessage(const std::string& msg)
  {
    if (stop_) {
      return false;
    }
    
    const boost::uint32_t messageHeader = (boost::uint32_t) msg.size();
    if( sizeof(messageHeader) + msg.size() > mMaxMsgSize )
    {
      //      LOG_ERR << "Message too big to be sent. Size: " << ((int)(sizeof(messageHeader) + message.getSize()));
      return false;
    }
    
    std::string tempData((char *)&messageHeader, sizeof(messageHeader));
    tempData.append(msg);    
    
    if(queue_ != 0)
    {
      try
      {
        queue_->send(&tempData[0], tempData.size(), 0);
        return true;
      }
      catch(boost::interprocess::interprocess_exception &ex)
      {
//        LOG_ERR << ex.what();
      }
    }
    return false;
  }
  
  

  
}//end name space pbrpcpp


