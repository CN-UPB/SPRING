#include <set>
#include <iostream>
#include <fstream>
#include <ctime>
#include "log.h"

#include "util.h"
#include "state.h"


namespace Log
{
std::ofstream out;
bool enabled;

void init(std::string filename)
{
	out.open(filename.c_str(), std::ofstream::out);
	enabled=true;
}

void enable()
{
	enabled=true;
}

void disable()
{
	enabled=false;
}

std::string timestamp()
{
	std::time_t t = std::time(NULL);
	char str[20];
	std::strftime(str,20,"%H:%M:%S",std::localtime(&t));
	//return std::asctime(std::localtime(&t));
	std::string result(str);
	return result;
}

void info(std::string s)
{
	if(enabled)
	{
		out << "Info (" << timestamp() << "): " << s << std::endl;
		out.flush();
	}
}

void info_stdout(std::string s)
{
	if(enabled)
		std::cout << "Info (" << timestamp() << "): " << s << std::endl;
}

void info_and_stdout(std::string s)
{
	info(s);
	info_stdout(s);
}

void debug(std::string s)
{
	if(enabled)
	{
		out << "Debug (" << timestamp() << "): " << s << std::endl;
		out.flush();
	}
}

void warning(std::string s)
{
	if(enabled)
	{
		out << "WARNING (" << timestamp() << "): " << s << std::endl;
		out.flush();
	}
}

void dump_system_state(State *s)
{
	if(!enabled)
		return;
	out << "System state (" << timestamp() << "):" << std::endl;
	out.flush();
}

void close()
{
	out.close();
}
};

