#ifndef STATE_H_
#define STATE_H_

#include <string>
#include <set>
#include <map>
#include "application.h"
#include "overlay.h"


struct State
{
	std::map<std::string,Application*> applications;
	std::set<Overlay *> overlays;
	State * copy(); // shallow copy applications, deep copy overlays
};


#endif /* STATE_H_ */
