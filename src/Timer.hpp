/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TIMER_HPP
#define	TIMER_HPP

#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "IoServiceInitializer.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <memory>

namespace pbrpcpp {

    template<typename T>
    class Timer {
    public:
        typedef typename boost::ptr_map<T, boost::asio::deadline_timer>::auto_type auto_type;

        ~Timer() {
            stop();
        }

        void add(const T& t, int timeoutMillis, boost::function< void () > timeoutHandler) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            std::auto_ptr< boost::asio::deadline_timer > pTimer = removeTimer(t);

            if ( pTimer.get() ) {
                boost::system::error_code ec;
                pTimer->cancel(ec);
            }

            pTimer.reset(new boost::asio::deadline_timer(io_service_intializer_.get_io_service(), boost::posix_time::milliseconds(timeoutMillis)));
            pTimer->async_wait(boost::bind(&Timer::handleTimeout, this, _1, t, timeoutHandler));

            timers_.insert(t, pTimer);
        }

        void cancel(const T& t) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            std::auto_ptr< boost::asio::deadline_timer > pTimer = removeTimer(t);
            if (pTimer.get()) {
                boost::system::error_code ec;
                pTimer->cancel(ec);
            }
        }

        void stop() {
            io_service_intializer_.stop();
        }
    private:

        void handleTimeout(const boost::system::error_code& ec, T t, boost::function< void () > timeoutHandler) {
            {
                boost::lock_guard< boost::mutex > guard(mutex_);
                removeTimer(t);
            }

            if (!ec) {
                timeoutHandler();
            }
        }

        std::auto_ptr< boost::asio::deadline_timer > removeTimer(const T& t) {

            typename boost::ptr_map<T, boost::asio::deadline_timer >::iterator iter = timers_.find(t);

            if (iter == timers_.end()) {
                return std::auto_ptr< boost::asio::deadline_timer >(0);
            }

            return std::auto_ptr< boost::asio::deadline_timer >(timers_.release(iter).release());

        }
    private:
        IoServiceInitializer io_service_intializer_;
        boost::mutex mutex_;

        boost::ptr_map<T, boost::asio::deadline_timer > timers_;
    };
}//end name space pbrpcpp

#endif	/* TIMER_HPP */

