#include <string>
#include <map>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "solver_milp.h"
#include "state.h"
#include "milp.h"
#include "application.h"
#include "overlay.h"
#include "substrate.h"
#include "util.h"
#include "log.h"
#include "options.h"
#include "statistics.h"


namespace SolverMilp
{
std::string lp_file="milp.lp";
std::string sol_file="milp.sol";

Milp * create_milp(State *s)
{
	Log::debug("MILP creation starts");
	Milp *milp=new Milp();

	//Create map of all components, all arcs, and all data sources
	std::map<std::string,Component*> C;
	std::map<std::string,Arc*> A;
	std::set<DataSource*> S;
	std::map<std::string,Application*>::iterator it_app;
	for(it_app=s->applications.begin();it_app!=s->applications.end();it_app++)
	{
		Application *app=it_app->second;
		std::set<Component*> comps=app->get_all_components();
		std::set<Component *>::iterator it_comp;
		for(it_comp=comps.begin();it_comp!=comps.end();it_comp++)
		{
			Component *comp=*it_comp;
			C[comp->get_id()]=comp;
		}
		std::set<Arc*> arcs=app->get_arcs();
		std::set<Arc*>::iterator it_arc;
		for(it_arc=arcs.begin();it_arc!=arcs.end();it_arc++)
		{
			Arc *arc=*it_arc;
			A[arc->id]=arc;
		}
		std::set<DataSource*> dss=app->get_data_sources();
		S.insert(dss.begin(),dss.end());
	}

	//Determine which components have an instance on which nodes
	std::set<std::pair<std::string,int> > has_instance; // (comp_id,node_id) pairs
	std::set<Overlay*>::iterator it_ol;
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay * ol=*it_ol;
		std::set<Instance*> insts=ol->get_instances();
		std::set<Instance*>::iterator it_ins;
		for(it_ins=insts.begin();it_ins!=insts.end();it_ins++)
		{
			Instance * inst=*it_ins;
			Component * comp=inst->comp;
			Node * node=inst->node;
			has_instance.insert(std::make_pair(comp->get_id(),node->id));
		}
	}
	Log::debug("Auxiliary data structures created");

	//Create variables
	Var *var;
	std::map<std::string,Component*>::iterator it_C;
	std::map<std::string,Arc*>::iterator it_A;
	std::set<Node*> nodes=SubstrateNetwork::get_nodes();
	std::set<Node*>::iterator it_node,it_node2;
	std::set<Link*> links=SubstrateNetwork::get_links();
	std::set<Link*>::iterator it_link;
	for(it_C=C.begin();it_C!=C.end();it_C++)
	{
		std::string comp_id=it_C->first;
		Component *comp=it_C->second;
		for(it_node=nodes.begin();it_node!=nodes.end();it_node++)
		{
			Node *node=*it_node;
			int node_id=node->id;
			var=new Var(VarName("x")+comp_id+Str(node_id),BIN);
			milp->add_var(var);
			var=new Var(VarName("delta")+comp_id+Str(node_id),BIN);
			milp->add_var(var);
			var=new Var(VarName("rho")+comp_id+Str(node_id),CONT);
			var->add_LB(0);
			milp->add_var(var);
			var=new Var(VarName("mu")+comp_id+Str(node_id),CONT);
			var->add_LB(0);
			milp->add_var(var);
			for(unsigned k=1;k<=comp->get_nr_inputs();k++)
			{
				var=new Var(VarName("Lambda")+comp_id+Str(node_id)+Str(k),CONT);
				var->add_LB(0);
				milp->add_var(var);
			}
			for(unsigned k=1;k<=comp->get_nr_outputs();k++)
			{
				var=new Var(VarName("Lambda_prime")+comp_id+Str(node_id)+Str(k),CONT);
				var->add_LB(0);
				milp->add_var(var);
			}
		}
	}
	for(it_node=nodes.begin();it_node!=nodes.end();it_node++)
	{
		Node *node=*it_node;
		int node_id=node->id;
		var=new Var(VarName("omega")+Str(node_id)+"CPU",BIN);
		milp->add_var(var);
		var=new Var(VarName("omega")+Str(node_id)+"mem",BIN);
		milp->add_var(var);
	}
	for(it_link=links.begin();it_link!=links.end();it_link++)
	{
		Link *link=*it_link;
		var=new Var(VarName("omega")+Str(link->id),BIN);
		milp->add_var(var);
	}
	for(it_A=A.begin();it_A!=A.end();it_A++)
	{
		std::string arc_id=it_A->first;
		//Arc *arc=it_A->second;
		for(it_node=nodes.begin();it_node!=nodes.end();it_node++)
		{
			Node *v1=*it_node;
			int v1_id=v1->id;
			for(it_node2=nodes.begin();it_node2!=nodes.end();it_node2++)
			{
				Node *v2=*it_node2;
				int v2_id=v2->id;
				var=new Var(VarName("y")+arc_id+Str(v1_id)+Str(v2_id),CONT);
				var->add_LB(0);
				milp->add_var(var);
				for(it_link=links.begin();it_link!=links.end();it_link++)
				{
					Link *link=*it_link;
					var=new Var(VarName("z")+arc_id+Str(v1_id)+Str(v2_id)+Str(link->id),CONT);
					var->add_LB(0);
					milp->add_var(var);
					var=new Var(VarName("zeta")+arc_id+Str(v1_id)+Str(v2_id)+Str(link->id),BIN);
					milp->add_var(var);
				}
			}
		}
	}
	var=new Var(VarName("psi")+"CPU",CONT);
	var->add_LB(0);
	milp->add_var(var);
	var=new Var(VarName("psi")+"mem",CONT);
	var->add_LB(0);
	milp->add_var(var);
	var=new Var(VarName("psi")+"dr",CONT);
	var->add_LB(0);
	milp->add_var(var);
	Log::debug("Variables created.");

	//Post constraints
	Constraint *constr;
	double M=10000, M1=1000000, M2=1000; //TODO: determine more appropriate values
	std::set<DataSource*>::iterator it_S;
	for(it_S=S.begin();it_S!=S.end();it_S++)
	{
		DataSource *ds=*it_S;
		Node *node=ds->node;
		Component *comp=ds->component;
		double lambda=ds->lambda;
		Var *var_x=milp->get_var(VarName("x")+comp->get_id()+Str(node->id));
		//(1)
		constr=new Constraint("",EQ,1);
		constr->add_term(var_x,1);
		milp->add_constraint(constr);
		Var *var_lambda=milp->get_var(VarName("Lambda_prime")+comp->get_id()+Str(node->id)+Str(1));
		//(2)
		constr=new Constraint("",EQ,lambda);
		constr->add_term(var_lambda,1);
		milp->add_constraint(constr);
	}
	Log::debug("Loop 1 ended");
	for(it_C=C.begin();it_C!=C.end();it_C++)
	{
		std::string comp_id=it_C->first;
		Component *comp=it_C->second;
		for(it_node=nodes.begin();it_node!=nodes.end();it_node++)
		{
			Node *node=*it_node;
			int node_id=node->id;
			Var *var_x=milp->get_var(VarName("x")+comp_id+Str(node_id));
			if(comp->get_nr_inputs()>0) // not a source component
			{
				//Log::debug("Handling non-source component "+comp_id);
				LinearComponent *lc;
				lc=(LinearComponent*)comp; //assumption: only linear processing components
				for(unsigned k=1;k<=lc->get_nr_outputs();k++)
				{
					std::vector<double> coeffs=lc->get_output_rate_coeffs(k);
					assert(coeffs.size()==lc->get_nr_inputs()+1);
					//(7)
					constr=new Constraint("",EQ,0);
					//constr=new Constraint("",EQ,0.0-coeffs[coeffs.size()-1]);
					Var *var_lambda=milp->get_var(VarName("Lambda_prime")+comp_id+Str(node_id)+Str(k));
					constr->add_term(var_lambda,-1);
					for(unsigned input=1;input<=lc->get_nr_inputs();input++)
					{
						var_lambda=milp->get_var(VarName("Lambda")+comp_id+Str(node_id)+Str(input));
						constr->add_term(var_lambda,coeffs[input-1]);
					}
					constr->add_term(var_x,coeffs[coeffs.size()-1]);
					milp->add_constraint(constr);
				}
				std::vector<double> coeffs=lc->get_cpu_size_coeffs();
				assert(coeffs.size()==lc->get_nr_inputs()+1);
				//(12)
				constr=new Constraint("",EQ,0);
				Var *var_rho=milp->get_var(VarName("rho")+comp_id+Str(node_id));
				constr->add_term(var_rho,1);
				for(unsigned input=1;input<=lc->get_nr_inputs();input++)
				{
					Var *var_lambda=milp->get_var(VarName("Lambda")+comp_id+Str(node_id)+Str(input));
					constr->add_term(var_lambda,-coeffs[input-1]);
				}
				constr->add_term(var_x,-coeffs[coeffs.size()-1]);
				milp->add_constraint(constr);
				//(13)
				coeffs=lc->get_mem_size_coeffs();
				assert(coeffs.size()==lc->get_nr_inputs()+1);
				constr=new Constraint("",EQ,0);
				Var *var_mu=milp->get_var(VarName("mu")+comp_id+Str(node_id));
				constr->add_term(var_mu,1);
				for(unsigned input=1;input<=lc->get_nr_inputs();input++)
				{
					Var *var_lambda=milp->get_var(VarName("Lambda")+comp_id+Str(node_id)+Str(input));
					constr->add_term(var_lambda,-coeffs[input-1]);
				}
				constr->add_term(var_x,-coeffs[coeffs.size()-1]);
				milp->add_constraint(constr);
			}
			for(unsigned k=1;k<=comp->get_nr_outputs();k++)
			{
				//(4)
				constr=new Constraint("",LE,0);
				Var *var_lambda=milp->get_var(VarName("Lambda_prime")+comp_id+Str(node_id)+Str(k));
				constr->add_term(var_lambda,1);
				constr->add_term(var_x,-M);
				milp->add_constraint(constr);
			}
			for(unsigned k=1;k<=comp->get_nr_inputs();k++)
			{
				//(3)
				constr=new Constraint("",LE,0);
				Var *var_lambda=milp->get_var(VarName("Lambda")+comp_id+Str(node_id)+Str(k));
				constr->add_term(var_lambda,1);
				constr->add_term(var_x,-M);
				milp->add_constraint(constr);
				//(8)
				constr=new Constraint("",EQ,0);
				constr->add_term(var_lambda,1);
				std::set<Arc*> arcs=comp->get_incoming_arcs();
				std::set<Arc*>::iterator it_arc;
				for(it_arc=arcs.begin();it_arc!=arcs.end();it_arc++)
				{
					Arc *arc=*it_arc;
					if(arc->ending_input==k)
					{
						for(it_node2=nodes.begin();it_node2!=nodes.end();it_node2++)
						{
							Node *node2=*it_node2;
							Var *var_y=milp->get_var(VarName("y")+arc->id+Str(node2->id)+Str(node_id));
							constr->add_term(var_y,-1);
						}
					}
				}
				milp->add_constraint(constr);
			}
			for(unsigned k=1;k<=comp->get_nr_outputs();k++)
			{
				//(9)
				constr=new Constraint("",EQ,0);
				Var *var_lambda_prime=milp->get_var(VarName("Lambda_prime")+comp_id+Str(node_id)+Str(k));
				constr->add_term(var_lambda_prime,1);
				std::set<Arc*> arcs=comp->get_outgoing_arcs();
				std::set<Arc*>::iterator it_arc;
				for(it_arc=arcs.begin();it_arc!=arcs.end();it_arc++)
				{
					Arc *arc=*it_arc;
					if(arc->starting_output==k)
					{
						for(it_node2=nodes.begin();it_node2!=nodes.end();it_node2++)
						{
							Node *node2=*it_node2;
							Var *var_y=milp->get_var(VarName("y")+arc->id+Str(node_id)+Str(node2->id));
							constr->add_term(var_y,-1);
						}
					}
				}
				milp->add_constraint(constr);
			}
			//(5)
			int x_bar=0;
			if(has_instance.find(std::make_pair(comp_id,node_id))!=has_instance.end())
				x_bar=1;
			constr=new Constraint("",LE,x_bar);
			constr->add_term(var_x,1);
			Var *var_delta=milp->get_var(VarName("delta")+comp_id+Str(node_id));
			constr->add_term(var_delta,-1);
			milp->add_constraint(constr);
			//(6)
			constr=new Constraint("",LE,-x_bar);
			constr->add_term(var_x,-1);
			constr->add_term(var_delta,-1);
			milp->add_constraint(constr);
		}
	}
	Log::debug("Loop 2 ended");
	std::set<Node*>::iterator it_v,it_v1,it_v2;
	Node *v,*v1,*v2;
	for(it_A=A.begin();it_A!=A.end();it_A++)
	{
		Arc * a=it_A->second;
		for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
		{
			v=*it_v;
			for(it_v1=nodes.begin();it_v1!=nodes.end();it_v1++)
			{
				v1=*it_v1;
				for(it_v2=nodes.begin();it_v2!=nodes.end();it_v2++)
				{
					v2=*it_v2;
					//(10)
					constr=new Constraint("",EQ,0);
					std::set<Link*>::iterator it_l;
					for(it_l=v->outgoing_links.begin();it_l!=v->outgoing_links.end();it_l++)
					{
						Link *l=*it_l;
						Var *var_z=milp->get_var(VarName("z")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id));
						constr->add_term(var_z,1);
					}
					for(it_l=v->incoming_links.begin();it_l!=v->incoming_links.end();it_l++)
					{
						Link *l=*it_l;
						Var *var_z=milp->get_var(VarName("z")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id));
						constr->add_term(var_z,-1);
					}
					if(v==v1 && v1!=v2)
					{
						Var *var_y=milp->get_var(VarName("y")+a->id+Str(v1->id)+Str(v2->id));
						constr->add_term(var_y,-1);
					}
					if(!(v1!=v2 && v==v2))
						milp->add_constraint(constr);
				}
				std::set<Link*>::iterator it_l;
				for(it_l=links.begin();it_l!=links.end();it_l++)
				{
					Link *l=*it_l;
					//(11)
					constr=new Constraint("",LE,0);
					Var *var_z=milp->get_var(VarName("z")+a->id+Str(v->id)+Str(v1->id)+Str(l->id));
					constr->add_term(var_z,1);
					Var *var_zeta=milp->get_var(VarName("zeta")+a->id+Str(v->id)+Str(v1->id)+Str(l->id));
					constr->add_term(var_zeta,-M);
					milp->add_constraint(constr);
				}
			}
		}
	}
	Log::debug("Loop 3 ended");
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		v=*it_v;
		//(14),(15)
		constr=new Constraint("",LE,v->cap_cpu);
		Constraint * constr_15=new Constraint("",LE,v->cap_cpu);
		for(it_C=C.begin();it_C!=C.end();it_C++)
		{
			std::string comp_id=it_C->first;
			Var *var_rho=milp->get_var(VarName("rho")+comp_id+Str(v->id));
			constr->add_term(var_rho,1);
			constr_15->add_term(var_rho,1);
		}
		Var *var_omega=milp->get_var(VarName("omega")+Str(v->id)+"CPU");
		constr->add_term(var_omega,-M);
		milp->add_constraint(constr);
		Var *var_psi=milp->get_var(VarName("psi")+"CPU");
		constr_15->add_term(var_psi,-1);
		milp->add_constraint(constr_15);
		//(16),(17)
		constr=new Constraint("",LE,v->cap_mem);
		Constraint * constr_17=new Constraint("",LE,v->cap_mem);
		for(it_C=C.begin();it_C!=C.end();it_C++)
		{
			std::string comp_id=it_C->first;
			Var *var_mu=milp->get_var(VarName("mu")+comp_id+Str(v->id));
			constr->add_term(var_mu,1);
			constr_17->add_term(var_mu,1);
		}
		var_omega=milp->get_var(VarName("omega")+Str(v->id)+"mem");
		constr->add_term(var_omega,-M);
		milp->add_constraint(constr);
		var_psi=milp->get_var(VarName("psi")+"mem");
		constr_17->add_term(var_psi,-1);
		milp->add_constraint(constr_17);
	}
	Log::debug("Loop 4 ended");
	std::set<Link*>::iterator it_l;
	for(it_l=links.begin();it_l!=links.end();it_l++)
	{
		Link *l=*it_l;
		//(18),(19)
		constr=new Constraint("",LE,l->max_data_rate);
		Constraint * constr_19=new Constraint("",LE,l->max_data_rate);
		for(it_A=A.begin();it_A!=A.end();it_A++)
		{
			Arc * a=it_A->second;
			for(it_v1=nodes.begin();it_v1!=nodes.end();it_v1++)
			{
				v1=*it_v1;
				for(it_v2=nodes.begin();it_v2!=nodes.end();it_v2++)
				{
					v2=*it_v2;
					Var *var_z=milp->get_var(VarName("z")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id));
					constr->add_term(var_z,1);
					constr_19->add_term(var_z,1);
				}
			}
		}
		Var *var_omega=milp->get_var(VarName("omega")+Str(l->id));
		constr->add_term(var_omega,-M);
		milp->add_constraint(constr);
		Var *var_psi=milp->get_var(VarName("psi")+"dr");
		constr_19->add_term(var_psi,-1);
		milp->add_constraint(constr_19);
	}
	Log::debug("Loop 5 ended");

	//Create objective function
	Objective obj(MIN);
	for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
	{
		v=*it_v;
		Var *var_omega=milp->get_var(VarName("omega")+Str(v->id)+"CPU");
		obj.add_term(var_omega,M1);
		var_omega=milp->get_var(VarName("omega")+Str(v->id)+"mem");
		obj.add_term(var_omega,M1);
	}
	Log::debug("Objective / Loop Obj1 ended");
	for(it_l=links.begin();it_l!=links.end();it_l++)
	{
		Link *l=*it_l;
		Var *var_omega=milp->get_var(VarName("omega")+Str(l->id));
		obj.add_term(var_omega,M1);
	}
	Log::debug("Objective / Loop Obj2 ended");
	for(it_C=C.begin();it_C!=C.end();it_C++)
	{
		std::string comp_id=it_C->first;
		for(it_node=nodes.begin();it_node!=nodes.end();it_node++)
		{
			Node *node=*it_node;
			int node_id=node->id;
			Var *var_rho=milp->get_var(VarName("rho")+comp_id+Str(node_id));
			obj.add_term(var_rho,1);
			Var *var_mu=milp->get_var(VarName("mu")+comp_id+Str(node_id));
			obj.add_term(var_mu,1);
			Var *var_delta=milp->get_var(VarName("delta")+comp_id+Str(node_id));
			obj.add_term(var_delta,M2);
		}
	}
	Log::debug("Objective / Loop Obj3 ended");
	for(it_l=links.begin();it_l!=links.end();it_l++)
	{
		Link *l=*it_l;
		for(it_A=A.begin();it_A!=A.end();it_A++)
		{
			Arc * a=it_A->second;
			for(it_v1=nodes.begin();it_v1!=nodes.end();it_v1++)
			{
				v1=*it_v1;
				for(it_v2=nodes.begin();it_v2!=nodes.end();it_v2++)
				{
					v2=*it_v2;
					Var *var_z=milp->get_var(VarName("z")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id));
					obj.add_term(var_z,1);
					Var *var_zeta=milp->get_var(VarName("zeta")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id));
					obj.add_term(var_zeta,M2*(l->delay));
				}
			}
		}
	}
	Log::debug("Objective / Loop Obj4 ended");
	Var *var_psi=milp->get_var(VarName("psi")+"CPU");
	obj.add_term(var_psi,1);
	var_psi=milp->get_var(VarName("psi")+"mem");
	obj.add_term(var_psi,1);
	var_psi=milp->get_var(VarName("psi")+"dr");
	obj.add_term(var_psi,1);
	milp->set_objective(obj);
	Log::info("created MILP with "+Str(milp->get_nr_vars())+" variables and "+Str(milp->get_nr_constrs())+" constraints");
	return milp;
}


void run_solver()
{
	int time_limit=atoi(Options::get_option_value("solver_time_limit").c_str());
	std::string command="gurobi_cl Threads=1 LogToConsole=0 TimeLimit="+Str(time_limit)+" ResultFile="+sol_file+" "+lp_file;
	system(command.c_str());
}


void read_solution(State *s)
{
	std::map<std::string,double> varvals;

	//Reading solution file
	std::ifstream file;
	file.open(sol_file.c_str());
	std::string line;
	while(getline(file,line))
	{
		if(line[0]=='#')
			continue;
		std::istringstream sline(line);
		std::string varname;
		double value;
		sline >> varname >> value;
		//Log::debug("var: "+varname+", value: "+Str(value));
		varvals[varname]=value;
	}
	file.close();
	Log::debug("varvals created");

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
	//Making changes
	std::set<Overlay*>::iterator it_ol;
	std::set<Instance*>::iterator it_ins;
	for(it_ol=s->overlays.begin();it_ol!=s->overlays.end();it_ol++)
	{
		Overlay *ol=*it_ol;
		//Removing unneeded instances
		Log::debug("Removing unneeded instances");
		std::set<Instance*> insts=ol->get_instances();
		for(it_ins=insts.begin();it_ins!=insts.end();it_ins++)
		{
			Instance *inst=*it_ins;
			VarName x=VarName("x")+inst->comp->get_id()+Str(inst->node->id);
			VarName delta=VarName("delta")+inst->comp->get_id()+Str(inst->node->id);
			if(varvals[delta.to_string()]==1 && varvals[x.to_string()]==0)
				ol->remove_instance(inst,false);
		}
		//Creating new instances
		Log::debug("Creating new instances");
		Application *app=ol->get_application();
		std::set<Component*> comps=app->get_all_components();
		std::set<Component*>::iterator it_c;
		for(it_c=comps.begin();it_c!=comps.end();it_c++)
		{
			Component *comp=*it_c;
			std::set<Node*> nodes=SubstrateNetwork::get_nodes();
			std::set<Node*>::iterator it_v;
			for(it_v=nodes.begin();it_v!=nodes.end();it_v++)
			{
				Node *v=*it_v;
				VarName x=VarName("x")+comp->get_id()+Str(v->id);
				VarName delta=VarName("delta")+comp->get_id()+Str(v->id);
				if(varvals[delta.to_string()]==1 && varvals[x.to_string()]==1)
					ol->create_instance(comp,v,false);
			}
		}
		//Adding new edges
		Log::debug("Adding new edges");
		std::set<Arc*> arcs=app->get_arcs();
		std::set<Arc*>::iterator it_a;
		for(it_a=arcs.begin();it_a!=arcs.end();it_a++)
		{
			Arc *a=*it_a;
			std::set<Node*> nodes=SubstrateNetwork::get_nodes();
			std::set<Node*>::iterator it_v1,it_v2;
			for(it_v1=nodes.begin();it_v1!=nodes.end();it_v1++)
			{
				Node *v1=*it_v1;
				for(it_v2=nodes.begin();it_v2!=nodes.end();it_v2++)
				{
					Node *v2=*it_v2;
					VarName y=VarName("y")+a->id+Str(v1->id)+Str(v2->id);
					if(varvals[y.to_string()]>epsilon)
					{
						Log::debug("Handling variable "+y.to_string());
						Instance *i1=ol->find_instance(a->starting_comp,v1);
						Log::debug("starting instance: "+i1->comp->get_id()+" on node "+Str(i1->node->id));
						Instance *i2=ol->find_instance(a->ending_comp,v2);
						Log::debug("ending instance: "+i2->comp->get_id()+" on node "+Str(i2->node->id));
						if(ol->find_edge(i1,i2,a)==NULL)
						{
							Log::debug("Edge must be created");
							ol->create_edge(i1,i2,a);
							Log::debug("Created edge");
						}
					}
				}
			}
		}
		//Modifying / removing existing edges
		Log::debug("Modifying / removing existing edges");
		std::set<Edge*> edges=ol->get_edges();
		std::set<Edge*>::iterator it_e;
		for(it_e=edges.begin();it_e!=edges.end();it_e++)
		{
			Edge *e=*it_e;
			Node *v1=e->starting_inst->node;
			Node *v2=e->ending_inst->node;
			Arc *a=e->arc;
			VarName y=VarName("y")+a->id+Str(v1->id)+Str(v2->id);
			Log::debug("Handling variable "+y.to_string());
			if(varvals[y.to_string()]<epsilon)
				ol->remove_edge(e);
			else
			{
				Flow *f=e->flow;
				f->strength=varvals[y.to_string()];
				//Removing existing flow values if now 0
				Log::debug("Removing existing flow values if now 0");
				std::map<Link*,double>::iterator it_f;
				for(it_f=f->values.begin();it_f!=f->values.end();)
				{
					Link *l=it_f->first;
					VarName z=VarName("z")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id);
					if(varvals[z.to_string()]<epsilon)
						f->values.erase(it_f++);
					else
						++it_f;
				}
				//Adding / modifying flow values if now non-0
				Log::debug("Adding / modifying flow values if now non-0");
				std::set<Link*> links=SubstrateNetwork::get_links();
				std::set<Link*>::iterator it_l;
				for(it_l=links.begin();it_l!=links.end();it_l++)
				{
					Link *l=*it_l;
					VarName z=VarName("z")+a->id+Str(v1->id)+Str(v2->id)+Str(l->id);
					if(varvals[z.to_string()]>epsilon)
						f->values[l]=varvals[z.to_string()];
				}
			}
		}
	}
}


void solve(State *s)
{
	Statistics::start_algorithm("MILP conversion");
	Milp *milp=create_milp(s);
	Log::info("MILP created");
	milp->write_problem(lp_file);
	Log::info("MILP written to file");
	delete milp;
	Statistics::stop_algorithm();
	Statistics::start_algorithm("MILP solving");
	run_solver();
	Statistics::stop_algorithm();
	Log::info("MILP solved");
	Statistics::start_algorithm("MILP conversion back");
	read_solution(s);
	Statistics::stop_algorithm();
	Log::info("MILP solution processed");
}

} // namespace
