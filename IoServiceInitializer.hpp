/* 
 * File:   IoServiceInitializer.hpp
 * Author: stou
 *
 * Created on April 2, 2013, 9:14 AM
 */

#ifndef IOSERVICEINITIALIZER_HPP
#define	IOSERVICEINITIALIZER_HPP

namespace pbrpcpp {
        class IoServiceInitializer{
        public:
		IoServiceInitializer()
                :stop_( false )
                {                    
			io_service_work_.reset( new boost::asio::io_service::work( io_service_ ) );
			thread_.reset( new boost::thread( boost::bind( &IoServiceInitializer::ioServiceRun, this ) ) );
		
                }
                
                ~IoServiceInitializer() {
                    stop();
                }
		
                void stop() {
                    if( stop_ ) {
                        return;
                    }
                    
                    stop_ = true;
                    io_service_.stop();
                    thread_->join();
                }
                
               boost::asio::io_service& get_io_service() {
                   return io_service_;
               } 
        private:
		void ioServiceRun() {
			
			for( ; !stop_; ) {
				try {
					io_service_.run();
				}catch( ... ) {
				}
			}
		}
        private:
		
                volatile bool stop_;
		boost::asio::io_service io_service_;
		boost::shared_ptr<boost::asio::io_service::work> io_service_work_;
		boost::shared_ptr<boost::thread> thread_;
	};
}

#endif	/* IOSERVICEINITIALIZER_HPP */

