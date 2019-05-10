#ifndef MILP_H_
#define MILP_H_

#include <string>
#include <vector>
#include <map>


typedef enum {INT,BIN,CONT} VarType;
typedef enum {EQ,LT,GT,LE,GE} ConstrType;
typedef enum {MAX,MIN} ObjType;



class VarName
{
private:
	std::string basename;
	std::vector<std::string> indexes;
public:
	VarName(std::string basename);
	VarName operator+(std::string index);
	std::string to_string();
};


class Var
{
private:
	VarName name;
	VarType type;
	bool has_LB;
	bool has_UB;
	double LB;
	double UB;
public:
	Var(VarName name,VarType type);
	void add_LB(double LB);
	void add_UB(double UB);
	std::string get_name();
	std::string get_bound_string();
	VarType get_type();
};


class Constraint
{
private:
	std::string label;
	std::map<Var*,double> terms;
	ConstrType type;
	double rhs;
	std::string relsymbol();
public:
	Constraint(std::string label, ConstrType type, double rhs);
	void add_term(Var *var, double coeff);
	std::string to_string();
};


class Objective
{
private:
	ObjType type;
	std::map<Var*,double> terms;
public:
	Objective(ObjType type=MIN);
	void add_term(Var *var, double coeff);
	std::string to_string();
};


class Milp
{
private:
	std::map<std::string,Var*> vars;
	std::vector<Constraint*> constrs;
	Objective obj;
public:
	Milp(){};
	~Milp();
	void add_var(Var * var);
	void add_constraint(Constraint * constr);
	void set_objective(Objective obj);
	void write_problem(std::string filename);
	Var * get_var(VarName varname);
	int get_nr_vars();
	int get_nr_constrs();
};

#endif /* MILP_H_ */
