#include <iostream>
#include <fstream>
#include "draw.h"
#include "state.h"
#include "substrate.h"
#include "util.h"


namespace Draw
{

void draw_overlays(State *s, std::string filename)
{
	std::ofstream file;
	file.open(filename.c_str());
	file << "digraph G {\n";
	std::set<Overlay*>::iterator it_ol;
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay *ol=*it_ol;
		std::set<Instance*> instances=ol->get_instances();
		std::set<Instance*>::iterator it_in;
		for(it_in=instances.begin();it_in!=instances.end();it_in++)
		{
			Instance *inst=*it_in;
			std::string comp_id=inst->comp->get_id();
			int node_id=inst->node->id;
			std::string label="\""+comp_id+"\\nCPU: "+Str(inst->get_cpu_size())+"\\nmem: "+Str(inst->get_mem_size())+"\"";
			file << "subgraph cluster_node" << node_id << " { " << comp_id << node_id << " [label=" << label << "]; }\n";
		}
		std::set<Edge*> edges=ol->get_edges();
		std::set<Edge*>::iterator it_e;
		for(it_e=edges.begin();it_e!=edges.end();it_e++)
		{
			Edge *e=*it_e;
			std::string comp1_id=e->starting_inst->comp->get_id();
			std::string comp2_id=e->ending_inst->comp->get_id();
			int node1_id=e->starting_inst->node->id;
			int node2_id=e->ending_inst->node->id;
			double val=e->flow->strength;
			file << comp1_id << node1_id << " -> " << comp2_id << node2_id << " [label=" << val <<",style=dashed]; \n";
			std::map<Link*,double> flow_vals=e->flow->values;
			std::map<Link*,double>::iterator it_fv;
			for(it_fv=flow_vals.begin();it_fv!=flow_vals.end();it_fv++)
			{
				Link *l=it_fv->first;
				double fval=it_fv->second;
				std::string v1="node"+Str(l->start_node->id);
				std::string v2="node"+Str(l->end_node->id);
				file << v1 << " -> " << v2 << " [label=" << fval <<"]; \n";
			}
		}
	}
	std::set<Node*> nodes=SubstrateNetwork::get_nodes();
	std::set<Node*>::iterator it_v;
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		Node *v=*it_v;
		double cpu=v->cap_cpu;
		double mem=v->cap_mem;
		int id=v->id;
		file << "subgraph cluster_node" << id << " { label=\"node " << id << "\\nCPU: " << cpu << ", mem: " << mem << "\"; }\n";
	}
	file << "}";
	file.close();
}

} // namespace Draw

