#include <string>
#include <sstream>
#include <cstdlib>
#include "util.h"


std::vector<double> string_to_vector(std::string str)
{
	std::vector<double> result;
	str.erase(0,1);
	str.erase(str.find(']'));
	std::stringstream ss(str);
	std::string s;
	while(getline(ss,s,','))
	{
		result.push_back(atof(s.c_str()));
	}
	return result;
}


std::vector<std::vector<double> > string_to_matrix(std::string str)
{
	std::vector<std::vector<double> > result;
	str.erase(0,1);
	str.erase(str.rfind(']'));
	std::stringstream ss(str);
	std::string s;
	while(getline(ss,s,';'))
	{
		result.push_back(string_to_vector(s));
	}
	return result;
}


std::ostream& operator << (std::ostream& os, const std::vector<double>& v)
{
    os << "[";
    for (typename std::vector<double>::const_iterator ii = v.begin(); ii != v.end(); ++ii)
    {
    	if(ii==v.begin())
            os << *ii;
    	else
    		os << " " << *ii;
    }
    os << "]";
    return os;
}
