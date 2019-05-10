#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>


namespace Options
{
	void read(std::string filename);
	std::string get_option_value(std::string option_key);
	void set_option(std::string key, std::string value);
};


#endif /* OPTIONS_H_ */
