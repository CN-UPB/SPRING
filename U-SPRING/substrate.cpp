#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <random>
#include "substrate.h"
#include "log.h"
#include "util.h"


namespace SubstrateNetwork
{
std::set<Node *> nodes;
std::set<Link *> links;
int node_id=0;
int link_id=0;

Node * create_node(double cap_cpu, double cap_mem)
{
	Node *node=new Node;
	node->id=node_id;
	node->cap_cpu=cap_cpu;
	node->cap_mem=cap_mem;
	nodes.insert(node);
	node_id++;
	return node;
}

Link * create_link(Node *start, Node *end, double max_dr, double delay)
{
	Link *link=new Link;
	link->id=link_id;
	link->start_node=start;
	link->end_node=end;
	link->max_data_rate=max_dr;
	link->delay=delay;
	links.insert(link);
	start->outgoing_links.insert(link);
	end->incoming_links.insert(link);
	link_id++;
	return link;
}

void remove_node(Node *node)
{
	std::set<Link *>::iterator it;
	for(it=node->outgoing_links.begin();it!=node->outgoing_links.end();it++)
	{
		Link *l=*it;
		remove_link(l);
	}
	for(it=node->incoming_links.begin();it!=node->incoming_links.end();it++)
	{
		Link *l=*it;
		remove_link(l);
	}
	nodes.erase(node);
}

void remove_link(Link *link)
{
	links.erase(link);
	link->start_node->outgoing_links.erase(link);
	link->end_node->incoming_links.erase(link);
}

std::set<Node *> get_nodes()
{
	return nodes;
}

std::set<Link *> get_links()
{
	return links;
}

void read_from_VNMP_file(std::string filename)
{
	std::ifstream file;
	file.open(filename.c_str());
	std::string line;
	enum {before_nodes,in_nodes,in_links,after_links} where_in_file=before_nodes;
	std::vector<Node*> node_vec;
	while(where_in_file!=after_links && getline(file,line))
	{
		switch(where_in_file)
		{
		case before_nodes:
			if(line.substr(0,10)=="# Vertices")
				where_in_file=in_nodes;
			break;
		case in_nodes:
			if(line.substr(0,6)=="# Arcs")
				where_in_file=in_links;
			else
			{
				std::istringstream iss(line);
				int id,cost,cpu,mem;
				iss >> id >> cost >> cpu;
				// cpu = uniform_random_number(1,5); //low node capacity
				cpu = uniform_random_number(10,50); //high node capacity
				mem=cpu;
				// SD
				// cpu = 10;
				// mem = 10;
				node_vec.push_back(create_node(cpu,mem));
				Log::info("Node capacity: "+Str(cpu));
			}
			break;
		case in_links:
			if(line.substr(0,10)=="# noSlices")
				where_in_file=after_links;
			else
			{
				std::istringstream iss(line);
				int v1id,v2id,bw,cost,delay;
				iss >> v1id >> v2id >> bw >> cost >> delay;
				Node *v1=node_vec[v1id];
				Node *v2=node_vec[v2id];
				// create_link(v1,v2,bw,delay);
				// SD
				// int linkcap = uniform_random_number(50,100); //low link capacity
				int linkcap = uniform_random_number(500,1000); //high link capacity
				create_link(v1,v2,linkcap,delay);
				Log::info("Link capacity: "+Str(linkcap));
			}
			break;
		case after_links:
			break;
		}
	}
	file.close();
	Log::info("Read in substrate network with "+Str(nodes.size())+" nodes and "+Str(links.size())+" links");
}

Node * get_random_node()
{
	std::set<Node*>::iterator it_v=nodes.begin();
	// std::advance(it_v,rand()%nodes.size());
	// SD
	std::advance(it_v,uniform_random_number(0,nodes.size()-1));
	return *it_v;
}

Node * get_node(int node_id)
{
	Node * result=NULL;
	std::set<Node*>::iterator it_v;
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		Node * node=*it_v;
		if(node->id==node_id)
		{
			result=node;
			break;
		}
	}
	return result;
}

int uniform_random_number(int min, int max)
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(min, max);

    return dist(rng);
}

} // namespace SubstrateNetwork
