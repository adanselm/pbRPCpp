/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef SHMRPCSERVER_HPP
#define	SHMRPCSERVER_HPP

#include "BaseRpcServer.hpp"

namespace pbrpcpp {
  class ShmConnection;

  class ShmRpcServer: public BaseRpcServer {
    public:
      ShmRpcServer( const string& segmentName );
      ~ShmRpcServer();
      void Run();
      void Shutdown();

    protected:
      virtual void sendResponse( int clientId, const string& msg );
    private:
      void messageReceived( const string& msg );
    private:
      string segmentName_;
      //socket to accept the client request
      scoped_ptr<ShmConnection> inQueue_;
      scoped_ptr<ShmConnection> outQueue_;
  };
}//end name space pbrpcpp

#endif	/* SHMRPCSERVER_HPP */

