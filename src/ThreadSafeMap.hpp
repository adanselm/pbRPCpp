/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef THREADSAFEMAP_HPP
#define	THREADSAFEMAP_HPP

#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/functional.hpp>

using std::map;

namespace pbrpcpp {

    template< typename Key, typename Value >
    class ThreadSafeMap {
    public:
        typedef typename map<Key, Value>::value_type value_type;
        
        
        ThreadSafeMap( const Value& defValue = Value() )
        :defValue_( defValue )
        {
            
        }
        
        explicit ThreadSafeMap( Value& defValue )
        :defValue_( defValue )
        {
            
        }
        
        
        Value& operator[]( const Key& k ) {
            boost::lock_guard< boost::mutex > guard(mutex_);
            return data_[k];
        }
        
        const Value& operator[]( const Key& k ) const {
            boost::lock_guard< boost::mutex > guard(mutex_);
            return data_[k];
        }
        
        std::pair< Value, bool > insert(const Key& k, const Value& v ) {
            return insert( value_type( k, v ));
        }
                
        std::pair< Value, bool > insert(const value_type& v ) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            std::pair< typename map< Key, Value >::iterator, bool > ret = data_.insert( v );
            if( ret.second ) {
                return std::make_pair( ret.first->second, true );
            } else {
                return std::make_pair( defValue_, false );
            }
        }

        Value erase( const Key& k ) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            typename map< Key, Value >::iterator iter = data_.find(k);

            if (iter == data_.end()) {
                return defValue_;
            }

            Value ret = iter->second;
            data_.erase(iter);
            return ret;
        }

        const Value& get( const Key& k ) const {
            boost::lock_guard< boost::mutex > guard(mutex_);

            typename map< Key, Value >::const_iterator iter = data_.find(k);

            return ( iter == data_.end()) ? defValue_ : iter->second;
        }
        
        bool contains( const Key& k ) const {
            boost::lock_guard< boost::mutex > guard(mutex_);
            
            return data_.find( k ) != data_.end();
        }
        
        int size() const {
            boost::lock_guard< boost::mutex > guard(mutex_);
            
            return data_.size();
        }
        
        bool empty() const {
            boost::lock_guard< boost::mutex > guard(mutex_);
            
            return data_.empty();
        }
        
        void for_each( boost::function< void ( Key& k, Value& v ) > f ) {
            map< Key, Value > tmp;
            {
                boost::lock_guard< boost::mutex > guard(mutex_);
                tmp = data_;
            }
            
            for( typename map< Key, Value >::iterator iter = tmp.begin(); iter != tmp.end(); iter++ ) {
                f( iter->first, iter->second );
            }
        }
        
        void for_each( boost::function< void ( const Key& k, const Value& v ) > f ) {
            map< Key, Value > tmp;
            {
                boost::lock_guard< boost::mutex > guard(mutex_);
                tmp = data_;
            }
            
            for( typename map< Key, Value >::const_iterator iter = tmp.begin(); iter != tmp.end(); iter++ ) {
                f( iter->first, iter->second );
            }
        }
                
        void erase_all( ) {
            boost::lock_guard< boost::mutex > guard(mutex_);
            
            data_.clear();
        }
        
        void erase_all( const boost::function< void ( Key& k, Value& v ) >& f ) {
            map< Key, Value > tmp;
            
            {
                boost::lock_guard< boost::mutex > guard(mutex_);
                tmp.swap( data_ );                
            }
            
            boost::function< void ( Key& k, Value& v ) > fun = f;
            
            for( typename map< Key, Value >::iterator iter = tmp.begin(); iter != tmp.end(); iter++ ) {
                
                if( !fun.empty() ) {
                    fun( iter->first, iter->second );
                }
            }
        }
    private:
        Value defValue_;
        //mutex to gaurd the clients_ acccess
        mutable boost::mutex mutex_;
        //the client data
        map< Key, Value > data_;
    };
}//end name space pbrpcpp
#endif	/* CLIENTDATAMANAGER_HPP */

