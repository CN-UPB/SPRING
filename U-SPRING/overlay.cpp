#include "overlay.h"
#include "substrate.h"
#include "application.h"
#include "statistics.h"
#include "log.h"
#include "util.h"


// Instance

std::vector<double> Instance::input_data_rates()
{
	std::vector<double> input_rates(comp->get_nr_inputs(),0);
	std::set<Edge*>::iterator it_e;
	for(it_e=incoming_edges.begin();it_e!=incoming_edges.end();it_e++)
	{
		Edge *e=*it_e;
		input_rates[e->arc->ending_input-1]+=e->flow->strength;
	}
	return input_rates;
}


double Instance::get_cpu_size()
{
	return comp->cpu_size(input_data_rates());
}


double Instance::get_mem_size()
{
	return comp->mem_size(input_data_rates());
}


double Instance::get_output_data_rate(int output)
{
	if(comp->is_source_component())
		return source_output_data_rate;
	else
	{
		ProcessingComponent * pc=(ProcessingComponent*)comp;
		return pc->output_rate(input_data_rates(),output);
	}
}


// Overlay

Overlay::Overlay(Application *app)
{
	this->app=app;
}


Instance * Overlay::create_instance(Component *comp, Node *node, bool temporary)
{
	Instance *inst=new Instance;
	inst->comp=comp;
	inst->node=node;
	instances.insert(inst);
	if(!temporary)
		Statistics::create_instance();
	Log::debug("Created instance for component "+comp->get_id()+" on node "+Str(node->id));
	return inst;
}


Edge * Overlay::create_edge(Instance *starting_inst, Instance *ending_inst, Arc *arc)
{
	Edge *edge=new Edge;
	edge->starting_inst=starting_inst;
	edge->ending_inst=ending_inst;
	edge->arc=arc;
	edge->flow=new Flow;
	edge->flow->edge=edge;
	edge->flow->strength=0;
	edges.insert(edge);
	starting_inst->outgoing_edges.insert(edge);
	ending_inst->incoming_edges.insert(edge);
	Log::debug("Created edge from component "+arc->starting_comp->get_id()+" on node "+Str(starting_inst->node->id)+" to component "+arc->ending_comp->get_id()+" on node "+Str(ending_inst->node->id));
	return edge;
}


void Overlay::remove_instance(Instance *inst, bool temporary)
{
	std::set<Edge *>::iterator it;
	for(it=inst->outgoing_edges.begin();it!=inst->outgoing_edges.end();it++)
	{
		Edge *e=*it;
		remove_edge(e);
	}
	for(it=inst->incoming_edges.begin();it!=inst->incoming_edges.end();it++)
	{
		Edge *e=*it;
		remove_edge(e);
	}
	instances.erase(inst);
	if(!temporary)
		Statistics::remove_instance();
}


void Overlay::remove_edge(Edge * edge)
{
	edges.erase(edge);
	edge->starting_inst->outgoing_edges.erase(edge);
	edge->ending_inst->incoming_edges.erase(edge);
}


std::set<Instance *> Overlay::get_instances()
{
	return instances;
}


std::set<Edge *> Overlay::get_edges()
{
	return edges;
}


Application * Overlay::get_application()
{
	return app;
}


Instance * Overlay::find_instance(Component *comp, Node *node)
{
	Instance *inst=NULL;
	std::set<Instance*>::iterator it_i;
	for(it_i=instances.begin();it_i!=instances.end();it_i++)
	{
		Instance *i=*it_i;
		if(i->comp==comp && i->node==node)
			inst=i;
	}
	return inst;
}


Edge * Overlay::find_edge(Instance *starting_inst, Instance *ending_inst, Arc *arc)
{
	Edge *result=NULL;
	std::set<Edge*>::iterator it_e;
	for(it_e=edges.begin();it_e!=edges.end();it_e++)
	{
		Edge *e=*it_e;
		if(e->starting_inst==starting_inst && e->ending_inst==ending_inst && e->arc==arc)
			result=e;
	}
	return result;
}


Overlay * Overlay::copy()
{
	Overlay * new_ol=new Overlay(this->app);
	std::set<Instance*>::iterator it_i;
	for(it_i=instances.begin();it_i!=instances.end();it_i++)
	{
		Instance *i=*it_i;
		new_ol->create_instance(i->comp,i->node,true);
	}
	std::set<Edge*>::iterator it_e;
	for(it_e=edges.begin();it_e!=edges.end();it_e++)
	{
		Edge *e=*it_e;
		Instance * i1=find_instance(e->starting_inst->comp,e->starting_inst->node);
		Instance * i2=find_instance(e->ending_inst->comp,e->ending_inst->node);
		Edge * new_e=new_ol->create_edge(i1,i2,e->arc);
		new_e->flow=e->flow;
	}
	return new_ol;
}
