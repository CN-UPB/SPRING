#ifndef EVENT_H_
#define EVENT_H_

#include <string>

struct State;


class Event
{
public:
	Event() {};
	virtual ~Event() {};
	virtual void execute(State *s)=0;
};


class NewAppEvent : public Event
{
private:
	std::string appname;
	std::string templatefilename;
public:
	NewAppEvent(std::string app, std::string templatefile);
	void execute(State *s);
};


class RemoveAppEvent : public Event
{
private:
	std::string appname;
public:
	RemoveAppEvent(std::string app);
	void execute(State *s);
};


class DataSourceEvent : public Event
{
private:
	int nodenum;
	std::string appname;
	std::string componentname;
	double datarate;
public:
	DataSourceEvent(int nodenum,std::string appname,std::string componentname,double datarate);
	void execute(State *s);
};


namespace EventHandler
{
	void read_events();
	void init();
	unsigned get_event_num();
	bool has_next_event();
	void execute_next_event(State *s);
}


#endif /* EVENT_H_ */
