#ifndef LOG_H_
#define LOG_H_

#include <string>
#include <sstream>

struct State;


namespace Log
{
	void init(std::string filename);
	void enable();
	void disable();
	void info(std::string s);
	void info_stdout(std::string s);
	void info_and_stdout(std::string s);
	void debug(std::string s);
	void warning(std::string s);
	void dump_system_state(State *s);
	void close();
};


#endif /* LOG_H_ */
