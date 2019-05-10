#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "application.h"
#include "substrate.h"
#include "util.h"
#include "log.h"


// Component

Component::Component(int nr_inputs, int nr_outputs, std::string name, bool source_comp)
{
	this->nr_inputs=nr_inputs;
	this->nr_outputs=nr_outputs;
	this->id=name;
	this->source_comp=source_comp;
}


Component::~Component()
{
}


std::string Component::get_id()
{
	return id;
}


bool Component::is_source_component()
{
	return source_comp;
}


void Component::insert_outgoing_arc(Arc *arc)
{
	outgoing_arcs.insert(arc);
}


void Component::insert_incoming_arc(Arc *arc)
{
	incoming_arcs.insert(arc);
}


void Component::erase_outgoing_arc(Arc *arc)
{
	outgoing_arcs.erase(arc);
}


void Component::erase_incoming_arc(Arc *arc)
{
	incoming_arcs.erase(arc);
}


void Component::remove_incident_arcs(Application *app)
{
	std::set<Arc *>::iterator it;
	for(it=incoming_arcs.begin();it!=incoming_arcs.end();it++)
	{
		Arc *arc=*it;
		app->remove_arc(arc);
	}
	for(it=outgoing_arcs.begin();it!=outgoing_arcs.end();it++)
	{
		Arc *arc=*it;
		app->remove_arc(arc);
	}
}


unsigned Component::get_nr_inputs()
{
	return nr_inputs;
}


unsigned Component::get_nr_outputs()
{
	return nr_outputs;
}


std::set<Arc*> Component::get_incoming_arcs()
{
	return incoming_arcs;
}


std::set<Arc*> Component::get_outgoing_arcs()
{
	return outgoing_arcs;
}


// SourceComponent

SourceComponent::SourceComponent(std::string name) : Component(0,1,name,true)
{
}


double SourceComponent::cpu_size(std::vector<double> input_rates)
{
	return 0;
}


double SourceComponent::mem_size(std::vector<double> input_rates)
{
	return 0;
}



// ProcessingComponent

ProcessingComponent::ProcessingComponent(int nr_inputs, int nr_outputs, std::string name)
	: Component(nr_inputs,nr_outputs,name,false)
{
}


ProcessingComponent::~ProcessingComponent()
{
}


// LinearComponent

LinearComponent::LinearComponent(int nr_inputs, int nr_outputs, std::string name, std::vector<double> cpu_size_coeffs, std::vector<double> mem_size_coeffs, std::vector<std::vector<double> > output_rate_coeffs)
	: ProcessingComponent(nr_inputs,nr_outputs,name)
{
	this->cpu_size_coeffs=cpu_size_coeffs;
	this->mem_size_coeffs=mem_size_coeffs;
	this->output_rate_coeffs=output_rate_coeffs;
}


double LinearComponent::cpu_size(std::vector<double> input_rates)
{
	assert(cpu_size_coeffs.size()==input_rates.size()+1);
	double result=cpu_size_coeffs[cpu_size_coeffs.size()-1];
	for(unsigned i=0;i<input_rates.size();i++)
		result+=cpu_size_coeffs[i]*input_rates[i];
	return result;
}


double LinearComponent::mem_size(std::vector<double> input_rates)
{
	assert(mem_size_coeffs.size()==input_rates.size()+1);
	double result=mem_size_coeffs[mem_size_coeffs.size()-1];
	for(unsigned i=0;i<input_rates.size();i++)
		result+=mem_size_coeffs[i]*input_rates[i];
	return result;
}


double LinearComponent::output_rate(std::vector<double> input_rates, int output)
{
	std::vector<double> coeffs=output_rate_coeffs[output-1];
	assert(coeffs.size()==input_rates.size()+1);
	double result=coeffs[coeffs.size()-1];
	for(unsigned i=0;i<input_rates.size();i++)
		result+=coeffs[i]*input_rates[i];
	Log::debug("Output rate on output "+Str(output)+": "+Str(result));
	Log::debug("input rates: "+Str(input_rates)+", coeffs: "+Str(coeffs));
	return result;
}


double LinearComponent::allowed_input_rate_increase(double allowed_cpu_increase, double allowed_mem_increase, int input)
{
	double r1=allowed_cpu_increase/cpu_size_coeffs[input-1];
	double r2=allowed_mem_increase/mem_size_coeffs[input-1];
	double result=r1;
	if(r2<r1)
		result=r2;
	return result;
}


double LinearComponent::cpu_size_increase(double input_rate_increase, int input)
{
	return input_rate_increase*cpu_size_coeffs[input-1];
}


double LinearComponent::mem_size_increase(double input_rate_increase, int input)
{
	return input_rate_increase*mem_size_coeffs[input-1];
}


std::vector<double> LinearComponent::get_cpu_size_coeffs()
{
	return cpu_size_coeffs;
}


std::vector<double> LinearComponent::get_mem_size_coeffs()
{
	return mem_size_coeffs;
}


std::vector<double> LinearComponent::get_output_rate_coeffs(int output)
{
	return output_rate_coeffs[output-1];
}



// Application

Application::Application(std::string name)
{
	this->name=name;
	arc_id=0;
}


void Application::add_component(Component *comp)
{
	if(comp->is_source_component())
		source_components.insert((SourceComponent *)comp);
	else
		processing_components.insert((ProcessingComponent *)comp);
}


void Application::remove_component(Component *comp)
{
	comp->remove_incident_arcs(this);
	std::set<DataSource *>::iterator it2;
	if(comp->is_source_component())
	{
		SourceComponent *scomp=(SourceComponent *)comp;
		for(it2=data_sources.begin();it2!=data_sources.end();it2++)
		{
			DataSource *ds=*it2;
			if(ds->component==scomp)
				remove_data_source(ds);
		}
	}
	if(comp->is_source_component())
		source_components.erase((SourceComponent *)comp);
	else
		processing_components.erase((ProcessingComponent *)comp);
}


Arc * Application::add_arc(Component *starting_comp, Component *ending_comp, int starting_output, int ending_input)
{
	Arc *arc=new Arc;
	arc_id++;
	arc->starting_comp=starting_comp;
	arc->ending_comp=ending_comp;
	arc->starting_output=starting_output;
	arc->ending_input=ending_input;
	arc->id=name+Str(arc_id);
	arcs.insert(arc);
	starting_comp->insert_outgoing_arc(arc);
	ending_comp->insert_incoming_arc(arc);
	return arc;
}


void Application::remove_arc(Arc *arc)
{
	arc->starting_comp->erase_outgoing_arc(arc);
	arc->ending_comp->erase_incoming_arc(arc);
	arcs.erase(arc);
}


void Application::add_data_source(DataSource *ds)
{
	data_sources.insert(ds);
}


void Application::remove_data_source(DataSource *ds)
{
	data_sources.erase(ds);
}


std::set<Component*> Application::get_all_components()
{
	std::set<Component*> comps;
	comps.insert(source_components.begin(),source_components.end());
	comps.insert(processing_components.begin(),processing_components.end());
	return comps;
}


std::set<SourceComponent*> Application::get_source_components()
{
	return source_components;
}


std::set<Arc*> Application::get_arcs()
{
	return arcs;
}


std::set<DataSource *> Application::get_data_sources()
{
	return data_sources;
}


void Application::read_template_from_file(std::string filename)
{
	std::ifstream file;
	file.open(filename.c_str());
	std::string line;
	enum {before_components,in_components,in_arcs,after_arcs} where_in_file=before_components;
	std::map<std::string,Component*> comps;
	while(where_in_file!=after_arcs && getline(file,line))
	{
		switch(where_in_file)
		{
		case before_components:
			if(line.substr(0,11)=="components:")
				where_in_file=in_components;
			break;
		case in_components:
			if(line.substr(0,5)=="arcs:")
				where_in_file=in_arcs;
			else
			{
				std::string comptype,compname;
				std::istringstream iss(line);
				iss >> comptype >> compname;
				Component *comp;
				if(comptype=="source")
				{
					comp=new SourceComponent(compname);
				}
				else if(comptype=="linear")
				{
					int nr_inputs,nr_outputs;
					std::string str_cpu_size_coeffs,str_mem_size_coeffs,str_output_rate_coeffs;
					iss >> nr_inputs >> nr_outputs >> str_cpu_size_coeffs >> str_mem_size_coeffs >> str_output_rate_coeffs;
					std::vector<double> cpu_size_coeffs=string_to_vector(str_cpu_size_coeffs);
					std::vector<double> mem_size_coeffs=string_to_vector(str_mem_size_coeffs);
					std::vector<std::vector<double> > output_rate_coeffs=string_to_matrix(str_output_rate_coeffs);
					comp=new LinearComponent(nr_inputs,nr_outputs,compname,cpu_size_coeffs,mem_size_coeffs,output_rate_coeffs);
				}
				else
					assert(0);
				comps[compname]=comp;
				add_component(comp);
			}
			break;
		case in_arcs:
			if(line.substr(0,3)=="eof")
				where_in_file=after_arcs;
			else
			{
				std::istringstream iss(line);
				std::string source_comp, target_comp;
				int source_output,target_input;
				iss >> source_comp >> source_output >> target_comp >> target_input;
				Component *c1=comps[source_comp];
				Component *c2=comps[target_comp];
				add_arc(c1,c2,source_output,target_input);
				Log::debug("Created arc with parameters: "+source_comp+", "+target_comp+", "+Str(source_output)+", "+Str(target_input));
			}
			break;
		case after_arcs:
			break;
		}
	}
	file.close();
	Log::info("Read in template with "+Str(comps.size())+" components and "+Str(arcs.size())+" arcs");
}


std::string Application::get_name()
{
	return name;
}
