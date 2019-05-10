#include <queue>
//#include <limits>
#include <algorithm>
#include "solver_heur.h"
#include "state.h"
#include "util.h"
#include "substrate.h"
#include "log.h"
#include "statistics.h"


namespace SolverHeur
{

std::map<Link*,double> used_bw;
std::map<Node*,double> used_cpu;
std::map<Node*,double> used_mem;

void compute_used_bw(State *s)
{
	std::set<Link*> links=SubstrateNetwork::get_links();
	std::set<Link*>::iterator it_l;
	for(it_l=links.begin();it_l!=links.end();it_l++)
	{
		Link *l=*it_l;
		used_bw[l]=0;
	}
	std::set<Overlay*>::iterator it_ol;
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay *ol=*it_ol;
		std::set<Edge*> edges=ol->get_edges();
		std::set<Edge*>::iterator it_e;
		for(it_e=edges.begin();it_e!=edges.end();it_e++)
		{
			Edge *e=*it_e;
			std::map<Link*,double>::iterator it_f;
			for(it_f=e->flow->values.begin();it_f!=e->flow->values.end();it_f++)
			{
				Link *l=it_f->first;
				double val=it_f->second;
				used_bw[l]+=val;
			}
		}
	}
}

void compute_used_cpu_mem(State *s)
{
	std::set<Node*> nodes=SubstrateNetwork::get_nodes();
	std::set<Node*>::iterator it_v;
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		Node *v=*it_v;
		used_cpu[v]=0;
		used_mem[v]=0;
	}
	std::set<Overlay*>::iterator it_ol;
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay *ol=*it_ol;
		std::set<Instance*> insts=ol->get_instances();
		std::set<Instance*>::iterator it_i;
		for(it_i=insts.begin();it_i!=insts.end();it_i++)
		{
			Instance *i=*it_i;
			used_cpu[i->node]+=i->get_cpu_size();
			used_mem[i->node]+=i->get_mem_size();
		}
	}
}

std::vector<Instance*> topsort(std::set<Instance*> instances)
{
	std::vector<Instance*> result;
	std::set<Instance*> processed;
	while(instances.size()>0)
	{
		Instance *i=*(instances.begin());
		bool found=false;
		do
		{
			Instance * bad_predec=NULL;
			std::set<Edge*>::iterator it_e;
			for(it_e=i->incoming_edges.begin();it_e!=i->incoming_edges.end();it_e++)
			{
				Edge *e=*it_e;
				if(processed.find(e->starting_inst)==processed.end())
				{
					bad_predec=e->starting_inst;
					break;
				}
			}
			if(bad_predec==NULL)
				found=true;
			else
				i=bad_predec;
		} while(!found);
		result.push_back(i);
		processed.insert(i);
		instances.erase(i);
	}
	return result;
}

struct ValuedNode
{
	Node * node;
	double bw;
	double delay;
	ValuedNode(Node * n,double b,double d) {node=n;bw=b;delay=d;};
};

bool operator<(const ValuedNode& a, const ValuedNode& b)
{
	if(a.bw<b.bw)
		return true;
	else if(a.delay>b.delay)
		return true;
	else
		return false;
	//return a.bw/(a.delay+1) < b.bw/(b.delay+1);
}

/**
 * Increase the strength of flow f in overlay ol, by at most cutoff.
 * If update_used==true, then used_bw, used_cpu, and used_mem are updated.
 * If delay!=NULL, it will contain the increase in total delay.
 * Returns the actual increase in flow strength.
 */
double incr_flow(Flow * f, Overlay *ol, double cutoff, bool update_used, double *delay)
{
	Log::debug("incr_flow, cutoff="+Str(cutoff));
	Instance *i1=f->edge->starting_inst;
	Instance *i2=f->edge->ending_inst;
	int i2_input=f->edge->arc->ending_input;
	double allowed_cpu_increase=i2->node->cap_cpu-used_cpu[i2->node];
	double allowed_mem_increase=i2->node->cap_mem-used_mem[i2->node];
	ProcessingComponent * pc=(ProcessingComponent*)(i2->comp);
	double d=pc->allowed_input_rate_increase(allowed_cpu_increase,allowed_mem_increase,i2_input);
	if(d<epsilon)
		d=0;
	if(d<cutoff)
		cutoff=d;
	Log::debug("cutoff="+Str(cutoff));
	if(cutoff<epsilon)
		return 0;
	double inc;
	//zero flow
	if(i1->node==i2->node)
	{
		inc=cutoff;
		if(delay!=NULL)
			*delay=0;
	}
	else
	{
		//BFS, but biased towards high bandwidth
		std::map<Node*,Link*> tree; //incoming link of the BFS tree for each node
		std::set<Node*> visited;
		//std::priority_queue<std::pair<double,Node*> > Q;
		std::priority_queue<ValuedNode> Q;
		//double M=std::numeric_limits<double>::max();
		//Q.push(std::make_pair(cutoff,i1->node));
		Q.push(ValuedNode(i1->node,cutoff,0));
		bool found=false;
		double max_bw;
		while(!found)
		{
			//std::pair<double,Node*> p=Q.top();
			ValuedNode p=Q.top();
			Q.pop();
			if(p.node==i2->node)
			{
				found=true;
				max_bw=p.bw;
				if(delay!=NULL)
					*delay=p.delay;
			}
			else
			{
				std::set<Link*> links=p.node->outgoing_links;
				std::set<Link*>::iterator it_l;
				for(it_l=links.begin();it_l!=links.end();it_l++)
				{
					Link *l=*it_l;
					if(visited.find(l->end_node)==visited.end())
					{
						double b=std::min(p.bw,l->max_data_rate-used_bw[l]);
						double dy=p.delay+l->delay;
						Q.push(ValuedNode(l->end_node,b,dy));
						tree[l->end_node]=l;
						visited.insert(l->end_node);
					}
				}
			}
		}
		inc=std::min(cutoff,max_bw);
		Node *v=i2->node;
		do
		{
			Link * l=tree[v];
			if(f->values.find(l)==f->values.end())
				f->values[l]=inc;
			else
				f->values[l]+=inc;
			if(update_used)
				used_bw[l]+=inc;
			v=l->start_node;
		}
		while(v!=i1->node);
	}
	f->strength+=inc;
	if(update_used)
	{
		used_cpu[i2->node]+=pc->cpu_size_increase(inc,i2_input);
		used_mem[i2->node]+=pc->mem_size_increase(inc,i2_input);
	}
	Log::debug("Found increase "+Str(inc));
	return inc;
}

/**
 * Create a new instance of component j in overlay ol, allowing high data rate from instance i.
 * Data rates higher than cutoff are not better than cutoff. a is the arc from c(i) to j.
 * The new instance is pushed to the end of toporder.
 * Returns the created flow from i to the new instance.
 */
Flow * create_instance_and_flow(Component * j, Instance * i, Overlay *ol, double cutoff, Arc *a,std::vector<Instance*>*toporder)
{
	ValuedNode best_node(NULL,-1,0);
	//double best_value=-1;
	Flow * best_flow;
	std::set<Node*> nodes=SubstrateNetwork::get_nodes();
	std::set<Node*>::iterator it_v;
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		Node * v=*it_v;
		Log::debug("Creating temporary instance and edge");
		Instance *temp_i=ol->create_instance(j,v,true);
		used_cpu[v]+=temp_i->get_cpu_size();
		used_mem[v]+=temp_i->get_mem_size();
		Edge *temp_e=ol->create_edge(i,temp_i,a);
		temp_e->flow->strength=0;
		double delay;
		double gamma=incr_flow(temp_e->flow,ol,cutoff,false,&delay);
		Log::debug("Gamma="+Str(gamma));
		ValuedNode vn(v,gamma,delay);
//		if(gamma>best_value)
		if(best_node<vn)
		{
			//best_value=gamma;
			best_node=vn;
			best_flow=temp_e->flow;
		}
		ol->remove_edge(temp_e);
		used_cpu[v]-=temp_i->get_cpu_size();
		used_mem[v]-=temp_i->get_mem_size();
		ol->remove_instance(temp_i,true);
		if(gamma>=cutoff-epsilon)
			break;
	}
	if(best_flow->strength<epsilon)
	{
		Log::warning("No positive flow found that would satisfy the capacity constraints. Allocating to a random neighbor instead.");
		std::set<Link*> links=i->node->outgoing_links;
		std::set<Link*>::iterator it_l=links.begin();
		std::advance(it_l,rand()%links.size());
		Link * link=*it_l;
		best_node.node=link->end_node;
		best_flow=new Flow;
		best_flow->strength=cutoff;
		best_flow->values[link]=cutoff;
	}
	Instance * new_i=ol->create_instance(j,best_node.node,false);
	Edge * e=ol->create_edge(i,new_i,a);
	e->flow=best_flow;
	e->flow->edge=e;
	std::map<Link*,double>::iterator it_f;
	for(it_f=e->flow->values.begin();it_f!=e->flow->values.end();it_f++)
	{
		Link *l=it_f->first;
		double val=it_f->second;
		if(used_bw.find(l)==used_bw.end())
			used_bw[l]=val;
		else
			used_bw[l]+=val;
	}
	used_cpu[best_node.node]+=new_i->get_cpu_size();
	used_mem[best_node.node]+=new_i->get_mem_size();
	toporder->push_back(new_i);
	Log::debug("Fixing instance on node "+Str(best_node.node->id)+" with flow strength "+Str(best_flow->strength));
	return best_flow;
}

/**
 * Increase the flows in bigPhi leaving the given output of instance i in overlay ol
 * by delta_lambda in total. If new instances are created, these are pushed to the end
 * of toporder.
 */
void increase(Instance *i,unsigned output,std::set<Flow*> bigPhi,double delta_lambda,Overlay *ol,std::vector<Instance*>*toporder)
{
	//ensure all arcs have a terminating instance
	std::set<Arc*> arcs=i->comp->get_outgoing_arcs();
	std::set<Arc*> good_arcs;
	std::set<Arc*>::iterator it_a;
	for(it_a=arcs.begin();it_a!=arcs.end();it_a++)
	{
		Arc * a=*it_a;
		if(a->starting_output==output)
		{
			good_arcs.insert(a);
			bool found_edge=false;
			std::set<Edge*> edges=i->outgoing_edges;
			std::set<Edge*>::iterator it_e;
			for(it_e=edges.begin();it_e!=edges.end();it_e++)
			{
				Edge *e=*it_e;
				if(e->arc==a)
				{
					found_edge=true;
					break;
				}
			}
			if(!found_edge)
			{
				Log::debug("Creating edge for arc");
				Component *j=a->ending_comp;
				Flow * f=create_instance_and_flow(j,i,ol,delta_lambda,a,toporder);
				bigPhi.insert(f);
				delta_lambda-=f->strength;
				Log::debug("After edge creation, delta_lambda="+Str(delta_lambda));
			}
		}
	}
	//increase existing flows
	std::set<Flow*>::iterator it_f;
	for(it_f=bigPhi.begin();it_f!=bigPhi.end();it_f++)
	{
		Flow * f=*it_f;
		Log::debug("Increasing existing flow");
		double d=incr_flow(f,ol,delta_lambda,true,NULL);
		delta_lambda-=d;
		if(delta_lambda<=epsilon)
			break;
	}
	Log::debug("After increasing existing flows, delta_lambda="+Str(delta_lambda));
	//further increase by creating new instances and flows
	while(delta_lambda>epsilon)
	{
		it_a=good_arcs.begin();
		std::advance(it_a,rand()%good_arcs.size());
		Arc *a=*it_a;
		Component *j=a->ending_comp;
		Log::debug("Creating new flow");
		Flow * f=create_instance_and_flow(j,i,ol,delta_lambda,a,toporder);
		delta_lambda-=f->strength;
	}
}

bool compare_edges(Edge*e1,Edge*e2)
{
	return e1->flow->strength<e2->flow->strength;
}

void decrease(std::vector<Edge*> bigEps,double delta_lambda,Overlay *ol)
{
	//sort the edges in increasing order of their flow strength
	std::sort(bigEps.begin(),bigEps.end(),compare_edges);
	unsigned index=0;
	while(index<bigEps.size() && bigEps[index]->flow->strength<delta_lambda+epsilon && delta_lambda>epsilon)
	{
		Edge *e=bigEps[index];
		Log::debug("Removing edge from "+e->arc->starting_comp->get_id()+" on node "+Str(e->starting_inst->node->id)+" to "+e->arc->ending_comp->get_id()+" on node "+Str(e->ending_inst->node->id));
		delta_lambda-=e->flow->strength;
		ol->remove_edge(e);
		index++;
	}
	if(delta_lambda>epsilon)
	{
		Edge *e=bigEps[index];
		Flow *f=e->flow;
		double factor=(f->strength-delta_lambda)/f->strength;
		Log::debug("Decreasing flow from "+e->arc->starting_comp->get_id()+" on node "+Str(e->starting_inst->node->id)+" to "+e->arc->ending_comp->get_id()+" on node "+Str(e->ending_inst->node->id)+" by a factor of "+Str(factor));
		f->strength*=factor;
		std::map<Link*,double>::iterator it_va;
		for(it_va=f->values.begin();it_va!=f->values.end();it_va++)
			it_va->second*=factor;
	}
	/*
	//reduce each flow in Phi
	Log::debug("Reducing flows");
	std::set<Flow*>::iterator it_f;
	for(it_f=bigPhi.begin();it_f!=bigPhi.end();it_f++)
	{
		Flow *f=*it_f;
		f->strength*=lambda_prime/lambda;
		std::map<Link*,double>::iterator it_va;
		for(it_va=f->values.begin();it_va!=f->values.end();it_va++)
		{
			used_bw[it_va->first]-=it_va->second*(1-lambda_prime/lambda);
			it_va->second*=lambda_prime/lambda;
		}
	}
	*/
}

void construction(State *s)
{
	Log::debug("Construction starts");
	//ensure there is an overlay for each application
	std::map<std::string,Application*>::iterator it_app;
	for(it_app=s->applications.begin();it_app!=s->applications.end();it_app++)
	{
		Application * app=it_app->second;
		bool exists_ol=false;
		std::set<Overlay*>::iterator it_ol;
		for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
		{
			Overlay *ol=*it_ol;
			if(ol->get_application()==app)
			{
				exists_ol=true;
				break;
			}
		}
		if(!exists_ol)
		{
			Overlay *ol=new Overlay(app);
			s->overlays.insert(ol);
			Log::debug("Overlay created");
		}
	}

	//ensure there is no overlay without application
	std::set<Overlay*>::iterator it_ol=s->overlays.begin();
	while(it_ol!=s->overlays.end())
	{
		std::set<Overlay*>::iterator curr_it_ol=it_ol++;
		Overlay *ol=*curr_it_ol;
		if(s->applications.find(ol->get_application()->get_name())==s->applications.end())
			s->overlays.erase(curr_it_ol);
	}

	//ensure all overlays are correct
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay *ol=*it_ol;
		//ensure data sources are mapped correctly
		std::set<DataSource*> dss=ol->get_application()->get_data_sources();
		std::set<DataSource*>::iterator it_ds;
		for(it_ds=dss.begin();it_ds!=dss.end();it_ds++)
		{
			DataSource *ds=*it_ds;
			Instance *i=ol->find_instance(ds->component,ds->node);
			if(i==NULL)
			{
				i=ol->create_instance(ds->component,ds->node,false);
				Log::debug("Source component created");
			}
			i->source_output_data_rate=ds->lambda;
		}
		//ensure there is no source instance without data source
		std::set<Instance*> insts=ol->get_instances();
		std::set<Instance*>::iterator it_in;
		for(it_in=insts.begin();it_in!=insts.end();it_in++)
		{
			Instance * inst=*it_in;
			if(inst->comp->is_source_component())
			{
				bool found_ds=false;
				Log::debug("Checking whether source component "+inst->comp->get_id()+" on node "+Str(inst->node->id)+" has an associated data source");
				for(it_ds=dss.begin();it_ds!=dss.end();it_ds++)
				{
					DataSource *ds=*it_ds;
					if(ds->component==inst->comp && ds->node==inst->node && ds->lambda>epsilon)
					{
						found_ds=true;
						Log::debug("Yes");
						break;
					}
				}
				if(!found_ds)
				{
					Log::debug("Removing source component "+inst->comp->get_id()+" from node "+Str(inst->node->id));
					ol->remove_instance(inst,false);
				}
			}
		}
		//check each instance in topological order
		std::vector<Instance*> toporder=topsort(ol->get_instances());
		for(unsigned it_i=0;it_i<toporder.size();it_i++)
		{
			Instance *i=toporder[it_i];
			Log::debug("Considering "+i->comp->get_id()+" on node "+Str(i->node->id));
			//if processing component and each input data rate is 0 -> remove
			if(!(i->comp->is_source_component()))
			{
				bool all_zero=true;
				std::vector<double> input_rates=i->input_data_rates();
				for(unsigned index=0;index<input_rates.size();index++)
				{
					if(input_rates[index]>epsilon)
					{
						all_zero=false;
						break;
					}
				}
				if(all_zero)
				{
					Log::debug("Removing instance "+i->comp->get_id()+" from node "+Str(i->node->id));
					ol->remove_instance(i,false);
					continue;
				}
			}
			for(unsigned k=1;k<=i->comp->get_nr_outputs();k++)
			{
				//determine Phi and lambda
				double lambda=0;
				std::set<Flow*> bigPhi;
				std::vector<Edge*> bigEps;
				std::set<Edge*>::iterator it_e;
				for(it_e=i->outgoing_edges.begin();it_e!=i->outgoing_edges.end();it_e++)
				{
					Edge *e=*it_e;
					if(e->arc->starting_output==k)
					{
						bigPhi.insert(e->flow);
						bigEps.push_back(e);
						lambda+=e->flow->strength;
					}
				}
				double lambda_prime=i->get_output_data_rate(k);
				Log::debug("For output "+Str(k)+", lambda="+Str(lambda)+", lambda'="+Str(lambda_prime));
				if(lambda_prime<lambda-epsilon)
				{
					decrease(bigEps,lambda-lambda_prime,ol);
				}
				else if(lambda<lambda_prime-epsilon)
				{
					Log::debug("Increasing flows");
					increase(i,k,bigPhi,lambda_prime-lambda,ol,&toporder);
				}
			}
		}
	}
	Log::debug("Construction ended");
}

/*
void optimization(State *s)
{
}
*/

void solve(State *s)
{
	Statistics::start_algorithm("Heuristic construction");
	compute_used_bw(s);
	compute_used_cpu_mem(s);
	construction(s);
	Statistics::stop_algorithm();
	/*
	Statistics::start_algorithm("Heuristic optimization");
	optimization(s);
	Statistics::stop_algorithm();
	*/
}

} // namespace SolverHeur

