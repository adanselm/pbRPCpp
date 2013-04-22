/**
 *          Copyright Springbeats Sarl 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file ../LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef METHODCALLIDGENERATOR_HPP
#define	METHODCALLIDGENERATOR_HPP

#include <string>
#include <sstream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>


using std::string;
using std::ostringstream;

namespace pbrpcpp {
    class MethodCallIDGenerator {
    public:
        static string generateID() {
            ostringstream out;
            boost::uuids::random_generator uuid_generator;

            boost::uuids::uuid uuid = uuid_generator();
            out << uuid;
            return out.str();
        }
    };

}//end name space pbrpcpp


#endif	/* METHODCALLUIDGENERATOR_H */

