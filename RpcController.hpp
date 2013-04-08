/* 
 * File:   RpcController.hpp
 * Author: Steven
 *
 * Created on March 31, 2013, 6:10 PM
 */

#ifndef RPCCONTROLLER_HPP
#define	RPCCONTROLLER_HPP

#include "Util.hpp"
#include <google/protobuf/service.h>
#include <set>
#include <string>

using std::set;
using std::string;
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
        void cancel();
        void setStartCancelCallback( Closure* callback );
        void serializeTo( ostream& out ) const;

        void parseFrom( istream& in );
    private:
        bool canceled_;
        bool failed_;
        set<Closure*> cancelCallbacks_;
        Closure* startCancelCallback_;
        string failedReason_;

        friend class BaseRpcChannel;
        friend class Util;
        friend class BaseRpcServer;

    };

}

#endif	/* RPCCONTROLLER_HPP */

