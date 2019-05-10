#ifndef SUBSTRATE_H_
#define SUBSTRATE_H_

#include <set>
#include <string>

struct Link;


struct Node
{
	int id;
	std::set<Link *> outgoing_links;
	std::set<Link *> incoming_links;
	double cap_cpu;
	double cap_mem;
};


struct Link
{
	int id;
	Node *start_node;
	Node *end_node;
	double max_data_rate;
	double delay;
};


namespace SubstrateNetwork
{
	Node * create_node(double cap_cpu, double cap_mem);
	Link * create_link(Node *start, Node *end, double max_dr, double delay);
	void remove_node(Node *node);
	void remove_link(Link *link);
	std::set<Node *> get_nodes();
	std::set<Link *> get_links();
	void read_from_VNMP_file(std::string filename);
	Node * get_random_node();
	Node * get_node(int node_id);
	int uniform_random_number(int min, int max);
}

#endif /* SUBSTRATE_H_ */
