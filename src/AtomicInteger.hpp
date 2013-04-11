/* 
 * File:   AtomicInteger.hpp
 * Author: Steven
 *
 * Created on April 10, 2013, 10:02 PM
 */

#ifndef ATOMICINTEGER_HPP
#define	ATOMICINTEGER_HPP

#include <boost/thread/mutex.hpp>

namespace pbrpcpp {

    template< typename T >
    class AtomicInteger {
    public:

        AtomicInteger()
        : value_() {
        }

        AtomicInteger(T v)
        : value_(v) {
        }
        

        T operator++() {
            boost::lock_guard< boost::mutex > guard(mutex_);

            return ++value_;

        }

        T operator++(int /*unused*/) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            return value_++;
        }

        T operator--() {
            boost::lock_guard< boost::mutex > guard(mutex_);

            return --value_;
        }

        T operator--(int /*unused*/) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            return value_--;
        }

        AtomicInteger& operator+=(const AtomicInteger& right) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            this->value_ += right.value_;

            return *this;
        }

        AtomicInteger& operator-=(const AtomicInteger& right) {
            boost::lock_guard< boost::mutex > guard(mutex_);

            this->value_ -= right.value_;

            return *this;
        }

        operator T() const {
            boost::lock_guard< boost::mutex > guard(mutex_);

            return value_;
        }
    private:
        mutable boost::mutex mutex_;
        T value_;
    };
}//end namespace pbrpcpp


#endif	/* ATOMICINTEGER_HPP */

