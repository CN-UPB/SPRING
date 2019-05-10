#ifndef UTIL_H_
#define UTIL_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

//#define epsilon 0.001
const float epsilon = 0.001;


std::ostream& operator << (std::ostream& os, const std::vector<double>& v);


template <typename T>
std::string Str( const T & t )
{
   std::ostringstream os;
   os << t;
   return os.str();
}


std::vector<double> string_to_vector(std::string str);
std::vector<std::vector<double> > string_to_matrix(std::string str);

#endif /* UTIL_H_ */
