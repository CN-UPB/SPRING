#include <iostream>
#include <fstream>
#include "milp.h"
#include "util.h"
#include "log.h"


// VarName

VarName::VarName(std::string basename)
{
	this->basename=basename;
}


VarName VarName::operator+(std::string index)
{
	VarName result(*this);
	result.indexes.push_back(index);
	return result;
}


std::string VarName::to_string()
{
	std::string name(basename);
	std::vector<std::string>::iterator it;
	for(it=indexes.begin();it!=indexes.end();it++)
		name+="_"+(*it);
	return name;
}



// Var

Var::Var(VarName n,VarType t) : name(n), type(t)
{
//	this->name=name;
//	this->type=type;
	has_LB=false;
	has_UB=false;
	LB=0;
	UB=0;
}


void Var::add_LB(double LB)
{
	has_LB=true;
	this->LB=LB;
}


void Var::add_UB(double UB)
{
	has_UB=true;
	this->UB=UB;
}


std::string Var::get_name()
{
	return name.to_string();
}


std::string Var::get_bound_string()
{
	std::string result;
	if(has_LB)
		result+=Str(LB)+" <= ";
	if(has_LB||has_UB)
		result+=get_name();
	if(has_UB)
		result+=" <= "+Str(UB);
	return result;
}


VarType Var::get_type()
{
	return type;
}


// Constraint

Constraint::Constraint(std::string label, ConstrType type, double rhs)
{
	this->label=label;
	this->type=type;
	this->rhs=rhs;
}


void Constraint::add_term(Var *var, double coeff)
{
	terms[var]=coeff;
}


std::string Constraint::to_string()
{
	std::string result;
	if(label!="")
		result=label+": ";
	std::map<Var*,double>::iterator it;
	for(it=terms.begin();it!=terms.end();it++)
	{
		if(it!=terms.begin())
			result+=" +";
		result+=" "+Str(it->second)+" "+it->first->get_name();
	}
	result+=" "+relsymbol()+" "+Str(rhs);
	return result;
}


std::string Constraint::relsymbol()
{
	std::string result;
	switch(type)
	{
	case EQ:
		result="=";
		break;
	case LT:
		result="<";
		break;
	case GT:
		result=">";
		break;
	case LE:
		result="<=";
		break;
	case GE:
		result=">=";
		break;
	}
	return result;
}



// Objective

Objective::Objective(ObjType type)
{
	this->type=type;
}


void Objective::add_term(Var *var, double coeff)
{
	terms[var]=coeff;
}


std::string Objective::to_string()
{
	std::string result;
	if(type==MIN)
		result="Minimize\n";
	else
		result="Maximize\n";
	std::map<Var*,double>::iterator it;
	for(it=terms.begin();it!=terms.end();it++)
	{
		if(it!=terms.begin())
			result+=" +";
		result+=" "+Str(it->second)+" "+it->first->get_name();
	}
	return result;
}



// Milp


Milp::~Milp()
{
	std::vector<Constraint*>::iterator it_c;
	for(it_c=constrs.begin();it_c!=constrs.end();it_c++)
	{
		Constraint *c=*it_c;
		delete c;
	}
	std::map<std::string,Var*>::iterator it_v;
	for(it_v=vars.begin();it_v!=vars.end();it_v++)
	{
		Var *v=it_v->second;
		delete v;
	}
}


void Milp::add_var(Var * var)
{
	vars[var->get_name()]=var;
}


void Milp::add_constraint(Constraint * constr)
{
	constrs.push_back(constr);
}


void Milp::set_objective(Objective obj)
{
	this->obj=obj;
}


void Milp::write_problem(std::string filename)
{
	Log::debug("start writing MILP to file");
	std::ofstream file;
	file.open(filename.c_str());
	file << obj.to_string() << std::endl;
	Log::debug("objective written");
	file << "Subject To\n";
	std::vector<Constraint*>::iterator itc;
	for(itc=constrs.begin();itc!=constrs.end();itc++)
	{
		Constraint *constr=*itc;
		file << constr->to_string() << std::endl;
	}
	Log::debug("constraints written");
	file << "Bounds\n";
	std::map<std::string,Var*>::iterator itv;
	for(itv=vars.begin();itv!=vars.end();itv++)
	{
		Var *var=itv->second;
		std::string bound_string=var->get_bound_string();
		if(bound_string!="")
			file << bound_string << std::endl;
	}
	Log::debug("bounds written");
	std::vector<Var*> int_vars,bin_vars;
	for(itv=vars.begin();itv!=vars.end();itv++)
	{
		Var *var=itv->second;
		switch(var->get_type())
		{
		case INT:
			int_vars.push_back(var);
			break;
		case BIN:
			bin_vars.push_back(var);
			break;
		case CONT:
			break;
		}
	}
	std::vector<Var*>::iterator itv2;
	if(int_vars.size()>0)
	{
		file << "General\n";
		for(itv2=int_vars.begin();itv2!=int_vars.end();itv2++)
		{
			Var *var=*itv2;
			file << " " << var->get_name();
		}
		file << std::endl;
	}
	if(bin_vars.size()>0)
	{
		file << "Binary\n";
		for(itv2=bin_vars.begin();itv2!=bin_vars.end();itv2++)
		{
			Var *var=*itv2;
			file << " " << var->get_name();
		}
		file << std::endl;
	}
	Log::debug("variables written");
	file << "End\n";
	file.close();
}


Var * Milp::get_var(VarName varname)
{
	return vars[varname.to_string()];
}


int Milp::get_nr_vars()
{
	return vars.size();
}


int Milp::get_nr_constrs()
{
	return constrs.size();
}


