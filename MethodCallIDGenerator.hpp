/* 
 * File:   MethodCallIDGenerator.hpp
 * Author: Steven
 *
 * Created on March 17, 2013, 11:20 PM
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

}


#endif	/* METHODCALLUIDGENERATOR_H */

