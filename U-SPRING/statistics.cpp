#include <iostream>
#include <fstream>
#include <ctime>
#include "statistics.h"
#include "state.h"
#include "substrate.h"
#include "log.h"
#include "util.h"
#include "options.h"
#include <chrono>

namespace Statistics
{

unsigned nr_inst_creation=0;
unsigned nr_inst_deletion=0;
//clock_t alg_start;
std::chrono::system_clock::time_point alg_start;
std::string alg_name;
std::vector<std::string> alg_names;
std::vector<double> alg_runtimes;
std::string now = "_"; 

void init()
{
	nr_inst_creation=0;
	nr_inst_deletion=0;

	time_t curr_time;
	tm * curr_tm;
	char date_string[100];
	char time_string[100];
	time(&curr_time);
	curr_tm = localtime(&curr_time);
	strftime(date_string,50,"%y%m%d",curr_tm);
	strftime(time_string,50,"%H%M%S",curr_tm);
	now.append(date_string);
	now.append("_");
	now.append(time_string);

}

void create_instance()
{
	nr_inst_creation++;
}

void remove_instance()
{
	nr_inst_deletion++;
}

void evaluate(State *s, std::string prefix)
{
	std::map<Node*,double> node_cpu_loads;
	std::map<Node*,double> node_mem_loads;
	std::map<Link*,double> link_loads;
	std::set<Node*> nodes=SubstrateNetwork::get_nodes();
	std::set<Link*> links=SubstrateNetwork::get_links();
	std::set<Node*>::iterator it_v;
	std::set<Link*>::iterator it_l;
	double total_latency=0;
	double psi_cpu=0, psi_mem=0, psi_dr=0;
	double total_cpu=0, total_mem=0, total_dr=0;
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		Node *v=*it_v;
		node_cpu_loads[v]=0;
		node_mem_loads[v]=0;
	}
	for(it_l=links.begin();it_l!=links.end();it_l++)
	{
		Link *l=*it_l;
		link_loads[l]=0;
	}
	std::set<Overlay*>::iterator it_ol;
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay * ol=*it_ol;
		std::set<Instance*> insts=ol->get_instances();
		std::set<Instance*>::iterator it_i;
		for(it_i=insts.begin();it_i!=insts.end();it_i++)
		{
			Instance *inst=*it_i;
			node_cpu_loads[inst->node]+=inst->get_cpu_size();
			node_mem_loads[inst->node]+=inst->get_mem_size();
			total_cpu+=inst->get_cpu_size();
			total_mem+=inst->get_mem_size();
		}
		std::set<Edge*> edges=ol->get_edges();
		std::set<Edge*>::iterator it_e;
		for(it_e=edges.begin();it_e!=edges.end();it_e++)
		{
			Edge *e=*it_e;
			std::map<Link*,double> values=e->flow->values;
			std::map<Link*,double>::iterator it_f;
			for(it_f=values.begin();it_f!=values.end();it_f++)
			{
				Link *l=it_f->first;
				link_loads[l]+=it_f->second;
				total_dr+=it_f->second;
				if(it_f->second>epsilon)
					total_latency+=l->delay;
			}
		}
	}
	unsigned nr_node_overloads=0;
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		Node *v=*it_v;
		if(node_cpu_loads[v]>v->cap_cpu+epsilon)
		{
			nr_node_overloads++;
			psi_cpu=std::max(psi_cpu,node_cpu_loads[v]-v->cap_cpu);
			Log::debug("Overload: node "+Str(v->id)+" has CPU size "+Str(node_cpu_loads[v])+" and capacity "+Str(v->cap_cpu));
		}
		if(node_mem_loads[v]>v->cap_mem+epsilon)
		{
			nr_node_overloads++;
			psi_mem=std::max(psi_mem,node_mem_loads[v]-v->cap_mem);
			Log::debug("Overload: node "+Str(v->id)+" has memory size "+Str(node_mem_loads[v])+" and capacity "+Str(v->cap_mem));
		}
	}
	unsigned nr_link_overloads=0;
	for(it_l=links.begin();it_l!=links.end();it_l++)
	{
		Link *l=*it_l;
		if(link_loads[l]>l->max_data_rate)
		{
			nr_link_overloads++;
			psi_dr=std::max(psi_dr,link_loads[l]-l->max_data_rate);
		}
	}
	std::ofstream out;

	out.open(Options::get_option_value("statistics_file").c_str() + now + Str(".csv"), std::ofstream::app);
	out << prefix << " ; ";
	out << nr_node_overloads << " ; ";
	out << nr_link_overloads << " ; ";
	out << total_latency << " ; ";
	out << nr_inst_creation << " ; ";
	out << nr_inst_deletion << " ; ";
	out << psi_cpu << " ; ";
	out << psi_mem << " ; ";
	out << psi_dr << " ; ";
	out << total_cpu << " ; ";
	out << total_mem << " ; ";
	out << total_dr << std::endl;
	out.close();
}

void evaluate_header()
{
	std::ofstream out;
	out.open(Options::get_option_value("statistics_file").c_str() + now + Str(".csv"), std::ofstream::out);
	out << "Alg ; Event ; ";
	out << "Node overloads ; ";
	out << "Link overloads ; ";
	out << "Total latency ; ";
	out << "Instance creations ; ";
	out << "Instance deletions ; ";
	out << "Max CPU overload ; ";
	out << "Max memory overload ; ";
	out << "Max data rate overload ; ";
	out << "Total CPU load ; ";
	out << "Total memory load ; ";
	out << "Total data rate" << std::endl;
	out.close();
}

void start_algorithm(std::string name)
{
	//alg_start=clock();
	alg_start = std::chrono::high_resolution_clock::now();
	alg_name=name;
}

void stop_algorithm()
{
	//clock_t time=clock()-alg_start;
	//double msec=(((float)time)/CLOCKS_PER_SEC)*1000;
	double msec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - alg_start).count();
	alg_names.push_back(alg_name);
	alg_runtimes.push_back(msec);
}

void print_alg_runtimes()
{
	std::ofstream out;
	out.open(Options::get_option_value("runtimes_file").c_str() + now + Str(".csv"), std::ofstream::out);
	out << "Algorithm ; Runtime" << std::endl;
	for(unsigned i=0;i<alg_names.size();i++)
	{
		out << alg_names[i] << " ; " << alg_runtimes[i] << std::endl;
	}
	out.close();
}

} // namespace Statistics

