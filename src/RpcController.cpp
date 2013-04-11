/* 
 * File:   RpcCController.cpp
 * Author: Steven
 *
 * Created on March 17, 2013, 11:13 PM
 */

#include "RpcController.hpp"
#include "Util.hpp"



namespace pbrpcpp {
    RpcController::RpcController()
    :canceled_( false ),
    failed_( false ),
    startCancelCallback_( 0 )
    {

    }
    /**
     * client side
     */
    void RpcController::Reset() {
        canceled_ = false;
        failed_ = false;
        failedReason_ = "";
        {
            boost::lock_guard< boost::mutex > guard( cancelCbMutex_ );
            cancelCallbacks_.clear();
        }
        startCancelCallback_ = 0;
    }

    bool RpcController::Failed() const {
        return failed_;
    }

    string RpcController::ErrorText() const {
        return failedReason_;
    }

    void RpcController::StartCancel() {
        if( startCancelCallback_  ) {
            startCancelCallback_->Run();
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

    void RpcController::cancel() {
        if( canceled_ ) {
            return;
        }

        canceled_ = true;

        set<Closure*> tmp; 
        {
            boost::lock_guard< boost::mutex > guard( cancelCbMutex_ );
            tmp = cancelCallbacks_;
            cancelCallbacks_.clear();
        }
        
        for( set<Closure*>::iterator iter = tmp.begin(); iter != tmp.end(); iter++ ) {
            (*iter)->Run();
        }
    }
    void RpcController::setStartCancelCallback( Closure* callback ) {
        startCancelCallback_ = callback;
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

