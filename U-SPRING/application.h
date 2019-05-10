#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <set>
#include <vector>
#include <string>

struct Arc;
struct Node;
class Application;


class Component
{
private:
	unsigned nr_inputs;
	unsigned nr_outputs;
	std::set<Arc *> outgoing_arcs;
	std::set<Arc *> incoming_arcs;
	std::string id; //must be unique!
	bool source_comp;
public:
	Component(int nr_inputs, int nr_outputs, std::string id, bool source_comp);
	virtual ~Component();
	std::string get_id();
	bool is_source_component();
	virtual double cpu_size(std::vector<double> input_rates)=0;
	virtual double mem_size(std::vector<double> input_rates)=0;
	void insert_outgoing_arc(Arc *arc);
	void insert_incoming_arc(Arc *arc);
	void erase_outgoing_arc(Arc *arc);
	void erase_incoming_arc(Arc *arc);
	void remove_incident_arcs(Application *app);
	unsigned get_nr_inputs();
	unsigned get_nr_outputs();
	std::set<Arc*> get_incoming_arcs();
	std::set<Arc*> get_outgoing_arcs();
};


class SourceComponent : public Component
{
public:
	SourceComponent(std::string name);
	double cpu_size(std::vector<double> input_rates);
	double mem_size(std::vector<double> input_rates);
};


class ProcessingComponent : public Component
{
public:
	ProcessingComponent(int nr_inputs, int nr_outputs, std::string name);
	virtual ~ProcessingComponent();
	virtual double cpu_size(std::vector<double> input_rates)=0;
	virtual double mem_size(std::vector<double> input_rates)=0;
	virtual double output_rate(std::vector<double> input_rates, int output)=0;
	virtual double allowed_input_rate_increase(double allowed_cpu_increase, double allowed_mem_increase, int input)=0;
	virtual double cpu_size_increase(double input_rate_increase, int input)=0;
	virtual double mem_size_increase(double input_rate_increase, int input)=0;
};


class LinearComponent : public ProcessingComponent
{
private:
	std::vector<double> cpu_size_coeffs;
	std::vector<double> mem_size_coeffs;
	std::vector<std::vector<double> > output_rate_coeffs;
public:
	LinearComponent(int nr_inputs, int nr_outputs, std::string name, std::vector<double> cpu_size_coeffs, std::vector<double> mem_size_coeffs, std::vector<std::vector<double> > output_rate_coeffs);
	double cpu_size(std::vector<double> input_rates);
	double mem_size(std::vector<double> input_rates);
	double output_rate(std::vector<double> input_rates, int output);
	double allowed_input_rate_increase(double allowed_cpu_increase, double allowed_mem_increase, int input);
	double cpu_size_increase(double input_rate_increase, int input);
	double mem_size_increase(double input_rate_increase, int input);
	std::vector<double> get_cpu_size_coeffs();
	std::vector<double> get_mem_size_coeffs();
	std::vector<double> get_output_rate_coeffs(int output);
};


struct Arc
{
	Component *starting_comp;
	Component *ending_comp;
	unsigned starting_output;
	unsigned ending_input;
	std::string id;
};


struct DataSource
{
	Node *node;
	SourceComponent *component;
	double lambda;
};


class Application
{
private:
	std::set<SourceComponent *> source_components;
	std::set<ProcessingComponent *> processing_components;
	std::set<Arc *> arcs;
	std::set<DataSource *> data_sources;
	std::string name;
	int arc_id;
public:
	Application(std::string name);
	void add_component(Component *comp);
	void remove_component(Component *comp);
	Arc * add_arc(Component *starting_comp, Component *ending_comp, int starting_output, int ending_input);
	void remove_arc(Arc *arc);
	void add_data_source(DataSource *ds);
	void remove_data_source(DataSource *ds);
	std::set<Component*> get_all_components();
	std::set<SourceComponent*> get_source_components();
	std::set<Arc*> get_arcs();
	std::set<DataSource *> get_data_sources();
	void read_template_from_file(std::string filename);
	std::string get_name();
};

#endif /* APPLICATION_H_ */
