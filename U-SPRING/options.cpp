#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "options.h"


namespace Options
{
std::map<std::string,std::string> option_map;

void read(std::string filename)
{
	std::ifstream infile(filename.c_str());
	std::string line;
	while(std::getline(infile,line))
	{
		std::string key, value;
		std::stringstream iss(line);
		std::getline(iss,key,'=');
		std::getline(iss,value);
		option_map.insert(std::pair<std::string,std::string>(key,value));
	}
	infile.close();
}


std::string get_option_value(std::string option_key)
{
	return option_map.find(option_key)->second;
}


void set_option(std::string key, std::string value)
{
	std::map<std::string,std::string>::iterator it;
	it=option_map.find(key);
	if(it==option_map.end())
		option_map.insert(std::pair<std::string,std::string>(key,value));
	else
		it->second=value;
}

} // namespace Options

