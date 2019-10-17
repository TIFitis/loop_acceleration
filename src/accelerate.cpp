/*
 * accelerate.cpp
 *
 *  Created on: 07-Oct-2019
 *      Author: akash
 */

#include "loop_acceleration.h"
#include "accelerator.h"

using namespace std;
using namespace loop_acc;

vector<goto_programt::targett> acceleratort::get_loops() {
	vector<goto_programt::targett> loops;
	forall_goto_functions(it, goto_model.goto_functions)
	{
		natural_loopst natural_loops;
		natural_loops(it->second.body);
		std::ostream outs(nullptr);
		outs.rdbuf(cout.rdbuf());
		natural_loops.output(outs);
	}
	return loops;
}
bool acceleratort::accelerate() {
	register_language(new_ansi_c_language);
//	register_language (new_cpp_language);
	get_loops();
	return false;
}
