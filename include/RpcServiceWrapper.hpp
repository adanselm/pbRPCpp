/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef libpbrpcpp_RpcServiceWrapper_hpp
#define libpbrpcpp_RpcServiceWrapper_hpp

#include <string>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>

namespace pbrpcpp
{
  
  template< typename ServiceImpl, typename RpcServer >
  class RpcServiceWrapper {
  public:
    
    RpcServiceWrapper(const boost::shared_ptr<RpcServer>& rpcServer,
                      int runDelay, bool exportEchoService = true)
    : rpcServer_(rpcServer),
    echoService_(runDelay)
    {
      if (exportEchoService)
      {
        rpcServer_->Export(&echoService_);
      }
    }
    
    ~RpcServiceWrapper()
    {
      stop();
    }
    
    void start()
    {
      stop();
      serverThread_ = boost::thread(boost::bind(&RpcServiceWrapper::run, this));
    }
    
    void stop()
    {
      rpcServer_->Shutdown();
      serverThread_.join();
    }
    
  private:
    void run()
    {
      rpcServer_->Run();
    }
    
  private:
    boost::shared_ptr<RpcServer> rpcServer_;
    ServiceImpl echoService_;
    boost::thread serverThread_;
  };
  
} // namespace end

#endif
