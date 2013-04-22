/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include "RpcController.hpp"
#include "Util.hpp"



namespace pbrpcpp {
    RpcController::RpcController()
    :canceled_( false ),
    failed_( false )
    {

    }
    /**
     * client side
     */
    void RpcController::Reset() {
        canceled_ = false;
        failed_ = false;
        failedReason_ = "";
        cancelFunc_.clear();
        {
            boost::lock_guard< boost::mutex > guard( cancelCbMutex_ );
            cancelCallbacks_.clear();
        }
    }

    bool RpcController::Failed() const {
        return failed_;
    }

    string RpcController::ErrorText() const {
        return failedReason_;
    }

    void RpcController::StartCancel() {
        if( !cancelFunc_.empty() ) {
          cancelFunc_();
        }
    }

    /**
     * Server side method 
     */
    void RpcController::SetFailed(const string& reason) {
        failed_ = true;
        failedReason_ = reason;
    }

    bool RpcController::IsCanceled() const {
        return canceled_;
    }

    void RpcController::NotifyOnCancel(Closure* callback) {
        //return if callback is NULL
        if( !callback ) {
            return;
        }

        //if already canceled, call immediately
        if( canceled_ ) {
            callback->Run();
        } else {
            boost::lock_guard< boost::mutex > guard( cancelCbMutex_ );
            
            cancelCallbacks_.insert( callback );
        }        
    }

    void RpcController::complete() {
        set<Closure*> tmp; 
        {
            boost::lock_guard< boost::mutex > guard( cancelCbMutex_ );
            tmp.swap( cancelCallbacks_ );
        }
        
        for( set<Closure*>::iterator iter = tmp.begin(); iter != tmp.end(); iter++ ) {
            (*iter)->Run();
        }
    }
    void RpcController::setStartCancelCallback( const boost::function<void()>& cancelFunc ) {
      cancelFunc_ = cancelFunc;
    }

    void RpcController::serializeTo( ostream& out ) const {
        Util::writeChar( canceled_ ? 'Y': 'N', out );
        Util::writeChar( failed_ ? 'Y': 'N', out );
        if( failed_ ) {
            Util::writeString( failedReason_, out );
        }
    }

    void RpcController::parseFrom( istream& in ) {
        canceled_ = (Util::readChar( in ) == 'Y');
        failed_ = (Util::readChar( in ) == 'Y');
        if( failed_ ) {
            failedReason_ = Util::readString( in );
        }
    }


}//end name space pbrpcpp

