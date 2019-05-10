#include "state.h"


State * State::copy()
{
	State * new_state=new State;
	new_state->applications=this->applications;
	std::set<Overlay *>::iterator it_ol;
	for(it_ol=overlays.begin();it_ol!=overlays.end();it_ol++)
	{
		Overlay * ol=*it_ol;
		new_state->overlays.insert(ol->copy());
	}
	return new_state;
}
