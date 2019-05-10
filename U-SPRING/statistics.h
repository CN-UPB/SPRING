#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <iostream>

struct State;


namespace Statistics
{
	void init();
	void create_instance();
	void remove_instance();
	void evaluate_header();
	void evaluate(State *s, std::string prefix);
	void start_algorithm(std::string name);
	void stop_algorithm();
	void print_alg_runtimes();
}


#endif /* STATISTICS_H_ */
