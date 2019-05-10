#ifndef OVERLAY_H_
#define OVERLAY_H_

#include <set>
#include <map>
#include <vector>

struct Edge;
struct Flow;
class Component;
class Application;
struct Arc;
struct Node;
struct Link;


struct Instance
{
	Component *comp;
	Node *node;
	std::set<Edge *> outgoing_edges;
	std::set<Edge *> incoming_edges;
	double source_output_data_rate; // only if this instance corresponds to a source component
	double get_cpu_size();
	double get_mem_size();
	double get_output_data_rate(int output);
	std::vector<double> input_data_rates();
};


struct Edge
{
	Instance *starting_inst;
	Instance *ending_inst;
	Arc *arc;
	Flow *flow;
};


struct Flow
{
	Edge *edge;
	std::map<Link*,double> values;
	double strength;
};


class Overlay
{
private:
	Application *app;
	std::set<Instance *> instances;
	std::set<Edge *> edges;
public:
	Overlay(Application *app);
	Instance * create_instance(Component *comp, Node *node, bool temporary);
	Edge * create_edge(Instance *starting_inst, Instance *ending_inst, Arc *arc);
	void remove_instance(Instance *inst, bool temporary);
	void remove_edge(Edge * edge);
	std::set<Instance *> get_instances();
	std::set<Edge *> get_edges();
	Application * get_application();
	Instance * find_instance(Component *comp, Node *node);
	Edge * find_edge(Instance *starting_inst, Instance *ending_inst, Arc *arc);
	Overlay * copy();
};

#endif /* OVERLAY_H_ */
