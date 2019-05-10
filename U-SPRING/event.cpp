#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "event.h"
#include "application.h"
#include "state.h"
#include "options.h"
#include "util.h"
#include "log.h"
#include "substrate.h"


// NewAppEvent methods

NewAppEvent::NewAppEvent(std::string app, std::string templatefile)
	: Event()
{
	appname=app;
	templatefilename=templatefile;
}

void NewAppEvent::execute(State *s)
{
	Application *app=new Application(appname);
	app->read_template_from_file(templatefilename);
	s->applications[appname]=app;
}


// RemoveAppEvent methods

RemoveAppEvent::RemoveAppEvent(std::string app)
	: Event()
{
	appname=app;
}

void RemoveAppEvent::execute(State *s)
{
	s->applications.erase(appname);
}


// DataSourceEvent methods

DataSourceEvent::DataSourceEvent(int nodenum,std::string appname,std::string componentname,double datarate)
	: Event()
{
	// this->nodenum=nodenum;
	// SD
	this->nodenum=SubstrateNetwork::get_random_node()->id;
	this->appname=appname;
	this->componentname=componentname;
	this->datarate=datarate;
}

void DataSourceEvent::execute(State *s)
{
	Application * app=s->applications[appname];
	std::set<DataSource*> dss=app->get_data_sources();
	DataSource * found_ds=NULL;
	std::set<DataSource*>::iterator it_ds;
	for(it_ds=dss.begin();it_ds!=dss.end();it_ds++)
	{
		DataSource * ds=*it_ds;
		if(ds->node->id==nodenum && ds->component->get_id()==componentname)
		{
			found_ds=ds;
			break;
		}
	}
	if(found_ds!=NULL)
	{
		if(datarate>epsilon)
		{
			found_ds->lambda=datarate;
			Log::debug("Setting data rate of source component "+found_ds->component->get_id()+" on node "+Str(found_ds->node->id)+" to "+Str(datarate));
		}
		else
		{
			app->remove_data_source(found_ds);
			Log::debug("Removing data source with component "+found_ds->component->get_id()+" on node "+Str(found_ds->node->id));
		}
	}
	else
	{
		DataSource * ds=new DataSource;
		ds->node=SubstrateNetwork::get_node(nodenum);
		ds->lambda=datarate;
		std::set<SourceComponent*> scs=app->get_source_components();
		std::set<SourceComponent*>::iterator it_sc;
		SourceComponent * found_sc=NULL;
		for(it_sc=scs.begin();it_sc!=scs.end();it_sc++)
		{
			SourceComponent * sc=*it_sc;
			if(sc->get_id()==componentname)
			{
				found_sc=sc;
				break;
			}
		}
		if(found_sc!=NULL)
			ds->component=found_sc;
		else
			Log::warning("Non-existing source component requested for data source: "+componentname);
		app->add_data_source(ds);
		Log::debug("Creating data source with source component "+ds->component->get_id()+" on node "+Str(ds->node->id)+" and data rate "+Str(datarate));
	}
}


namespace EventHandler
{

unsigned event_num;
std::vector<Event*> events;

void read_events()
{
	std::string filename=Options::get_option_value("events_file");
	std::ifstream file;
	file.open(filename.c_str());
	std::string line;
	enum {before_events,in_events,after_events} where_in_file=before_events;
	while(where_in_file!=after_events && getline(file,line))
	{
		switch(where_in_file)
		{
		case before_events:
			if(line.substr(0,7)=="events:")
				where_in_file=in_events;
			break;
		case in_events:
			if(line.substr(0,3)=="eof")
				where_in_file=after_events;
			else
			{
				std::string event_type;
				std::istringstream iss(line);
				iss >> event_type;
				Event *e;
				if(event_type=="new_app")
				{
					std::string appname,templatefilename;
					iss >> appname >> templatefilename;
					e=new NewAppEvent(appname,templatefilename);
				}
				else if(event_type=="remove_app")
				{
					std::string appname;
					iss >> appname;
					e=new RemoveAppEvent(appname);
				}
				else if(event_type=="data_source")
				{
					std::string appname,componentname;
					int nodenum;
					double datarate;
					iss >> nodenum >> appname >> componentname >> datarate;
					e=new DataSourceEvent(nodenum,appname,componentname,datarate);
				}
				else
					Log::warning("Unknown event type: "+event_type);
				events.push_back(e);
			}
		case after_events:
			break;
		}
	}
	file.close();
}

void init()
{
	event_num=0;
}

unsigned get_event_num()
{
	return event_num;
}

bool has_next_event()
{
	return event_num<events.size();
}

void execute_next_event(State *s)
{
	if(event_num<events.size())
	{
		events[event_num]->execute(s);
		event_num++;
	}
	else
		Log::warning("execute_next_event called, but there is no more event");
}

} // namespace EventHandler
