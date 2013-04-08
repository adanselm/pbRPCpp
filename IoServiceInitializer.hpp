/* 
 * File:   IoServiceInitializer.hpp
 * Author: Steven
 *
 * Created on April 2, 2013, 9:14 AM
 */

#ifndef IOSERVICEINITIALIZER_HPP
#define	IOSERVICEINITIALIZER_HPP

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace pbrpcpp {

    class IoServiceInitializer {
    public:

        IoServiceInitializer()
        : stop_(false),
        io_service_work_(io_service_),
        thread_(boost::bind(&IoServiceInitializer::ioServiceRun, this)) {
        }

        ~IoServiceInitializer() {
            stop();
        }

        void stop() {
            if (stop_) {
                return;
            }

            stop_ = true;
            io_service_.stop();
            thread_.join();
        }

        boost::asio::io_service& get_io_service() {
            return io_service_;
        }
        
        bool stopped() const {
            return stop_;
        }
    private:

        void ioServiceRun() {

            for (; !stop_;) {
                try {
                    io_service_.run();
                } catch (...) {
                }
            }
        }
    private:
        volatile bool stop_;
        boost::asio::io_service io_service_;
        boost::asio::io_service::work io_service_work_;
        boost::thread thread_;
    };
}//end name space pbrpcpp

#endif	/* IOSERVICEINITIALIZER_HPP */

