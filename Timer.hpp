/* 
 * File:   Timer.hpp
 * Author: Steven
 *
 * Created on April 1, 2013, 6:01 PM
 */

#ifndef TIMER_HPP
#define	TIMER_HPP

#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include "IoServiceInitializer.hpp"

using std::map;

namespace pbrpcpp {
    
    template<typename T>
    class Timer {
        
    public:                
        
        ~Timer() {
            stop();
        }
        
        void add( const T& t, int timeoutMillis, boost::function< void () > timeoutHandler ) {
            boost::asio::deadline_timer* pTimer = 0;
            
            {
                boost::lock_guard< boost::mutex > guard( mutex_ );
                
                pTimer = removeTimer( t );
                
                if( pTimer ) {
                    boost::system::error_code ec;
                    pTimer->cancel( ec );
                    delete pTimer;
                }

                pTimer = new boost::asio::deadline_timer( io_service_intializer_.get_io_service(), boost::posix_time::milliseconds( timeoutMillis ) );;
                timers_[ t ] = pTimer;
            }
            
            pTimer->async_wait( boost::bind( &Timer::handleTimeout, this, _1, t, timeoutHandler ) );            
        }
        
        void cancel( const T& t ) {
            boost::lock_guard< boost::mutex > guard( mutex_ );
            
            boost::asio::deadline_timer* pTimer = removeTimer( t );
            
            if( pTimer ) {
                boost::system::error_code ec;
                pTimer->cancel( ec );
                delete pTimer;
            }
        }
        
        void stop() {
            io_service_intializer_.stop();
        }
    private:
        void handleTimeout( const boost::system::error_code& ec, T t, boost::function< void () > timeoutHandler ) {
            {
                boost::lock_guard< boost::mutex > guard(mutex_);

                boost::asio::deadline_timer* pTimer = removeTimer( t );
                delete pTimer;
            }

            if (!ec) {
                timeoutHandler();
            }
        }
        
        boost::asio::deadline_timer* removeTimer( const T& t ) {
            typename map< T, boost::asio::deadline_timer* >::iterator iter = timers_.find(t);
            
            if (iter == timers_.end()) {
                return 0;
            }
            
            boost::asio::deadline_timer* pTimer = iter->second;
            
            timers_.erase( iter );
            
            return pTimer;
        }
    private:
        IoServiceInitializer io_service_intializer_;
        boost::mutex mutex_;
        map< T, boost::asio::deadline_timer* > timers_;
    };
}

#endif	/* TIMER_HPP */

