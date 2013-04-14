/* 
 * File:   RpcController.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 6:10 PM
 */

#ifndef RPCCONTROLLER_HPP
#define	RPCCONTROLLER_HPP

#include <google/protobuf/service.h>
#include <set>
#include <string>
#include <istream>
#include <ostream>
#include <boost/thread.hpp>
#include <boost/functional.hpp>

using std::set;
using std::string;
using std::istream;
using std::ostream;
using google::protobuf::Closure;

namespace pbrpcpp {
    
    class BaseRpcChannel;
    class BaseRpcServer;
    class Util;

    class RpcController: public google::protobuf::RpcController {
    public:
        RpcController();
        /**
         * client side
         */
        virtual void Reset();

        virtual bool Failed() const;
        virtual string ErrorText() const ;
        virtual void StartCancel() ;

        /**
         * Server side method 
         */
        virtual void SetFailed(const string& reason) ;
        virtual bool IsCanceled() const;

        virtual void NotifyOnCancel(Closure* callback);
    private:
        void complete();
        void setStartCancelCallback( const boost::function<void()>& cancelFunc );
        void serializeTo( ostream& out ) const;

        void parseFrom( istream& in );
    private:
        bool canceled_;
        // set to true if SetFailed is called
        bool failed_;
        boost::mutex cancelCbMutex_;
        // set by NotifyOnCancel() method
        set<Closure*> cancelCallbacks_;
        // Will be invoked if StartCancel is called
        boost::function<void()> cancelFunc_;
        // the failed reason set by SetFailed() method
        string failedReason_;

        friend class BaseRpcChannel;
        friend class Util;
        friend class BaseRpcServer;

    };

}//end name space pbrpcpp

#endif	/* RPCCONTROLLER_HPP */

