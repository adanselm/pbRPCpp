/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef SHMRPCCHANNEL_HPP
#define	SHMRPCCHANNEL_HPP

#include "BaseRpcChannel.hpp"
#include <boost/smart_ptr.hpp>

using boost::shared_ptr;
using boost::scoped_ptr;

namespace pbrpcpp {
  class ShmConnection;

    class ShmRpcChannel: public BaseRpcChannel {
    public:
        ShmRpcChannel( const string& segmentName );
        ~ShmRpcChannel();
        
        void close();
    protected:
        void sendMessage( const string& msg,  boost::function< void (bool, const string&) > resultCb );
    private:
        void messageReceived(const string& msg);
    private:
        bool stop_;
        scoped_ptr<ShmConnection> inQueue_;
        scoped_ptr<ShmConnection> outQueue_;
    };
}//end name space pbrpcpp

#endif	/* SHMRPCCHANNEL_HPP */

