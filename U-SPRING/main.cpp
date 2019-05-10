#include <iostream>
#include <cstdlib>
#include "overlay.h"
#include "substrate.h"
#include "application.h"
#include "state.h"
#include "solver_milp.h"
#include "solver_heur.h"
#include "log.h"
#include "draw.h"
#include "statistics.h"
#include "util.h"
#include "options.h"
#include "event.h"


int main()
{
	Options::read("options.ini");
	if(Options::get_option_value("logging")=="on")
		Log::init(Options::get_option_value("log_file"));
	else
		Log::disable();
	// srand(atoi(Options::get_option_value("random_seed").c_str()));
	srand(time(NULL));
	SubstrateNetwork::read_from_VNMP_file(Options::get_option_value("substrate_network_file"));

	/*
	Application app("App");
	app.read_template_from_file(Options::get_option_value("template_file"));
	std::set<SourceComponent*> source_components=app.get_source_components();
	std::set<SourceComponent*>::iterator it_sc;
	for(it_sc=source_components.begin();it_sc!=source_components.end();it_sc++)
	{
		SourceComponent *sc=*it_sc;
		DataSource *ds=new DataSource;
		ds->component=sc;
		//ds->node=SubstrateNetwork::get_random_node();
		ds->node=SubstrateNetwork::get_node(0);
		int min_lambda=atoi(Options::get_option_value("source_min_lambda").c_str());
		int max_lambda=atoi(Options::get_option_value("source_max_lambda").c_str());
		ds->lambda=min_lambda+rand()%(max_lambda-min_lambda);
		app.add_data_source(ds);
		Log::debug("Adding data source with comp="+sc->get_id()+", node="+Str(ds->node->id)+", lambda="+Str(ds->lambda));
	}

	//Overlay ol(&app);

	State s,*s1,*s2;
	s.applications["App"]=&app;
	//s.overlays.insert(&ol);
*/
	State s,*s1,*s2;
	s1=&s;
	s2=s.copy();
	EventHandler::read_events();
	EventHandler::init();
	Log::info("Problem created.");
	Statistics::evaluate_header();
	std::string alg=Options::get_option_value("algorithm");
	if(alg=="milp" || alg=="both")
	{
		Statistics::init();
		while(EventHandler::has_next_event())
		{
			Log::info_and_stdout("Executing MILP for event "+Str(EventHandler::get_event_num()+1));
			EventHandler::execute_next_event(s1);
			SolverMilp::solve(s1);
			Statistics::evaluate(s1,"milp;"+Str(EventHandler::get_event_num()));
			//Draw::draw_overlays(s1,"state_milp.dot");
			//system("dot -Tpdf state_milp.dot > state_milp.pdf");
		}
	}
	if(alg=="heuristic" || alg=="both")
	{
		Statistics::init();
		EventHandler::init();
		while(EventHandler::has_next_event())
		{
			Log::info_and_stdout("Executing heuristic for event "+Str(EventHandler::get_event_num()+1));
			EventHandler::execute_next_event(s2);
			SolverHeur::solve(s2);
			Statistics::evaluate(s2,"heuristic;"+Str(EventHandler::get_event_num()));
			//Draw::draw_overlays(s2,"state_heur.dot");
			//system("dot -Tpdf state_heur.dot > state_heur.pdf");
		}
	}
	Statistics::print_alg_runtimes();
	Log::info("Done");
	Log::close();
	return 0;
}
