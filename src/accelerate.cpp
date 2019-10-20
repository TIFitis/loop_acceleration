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

void acceleratort::get_loops() {
	Forall_goto_functions(it, goto_model.goto_functions)
	{
		auto natural_loops = new natural_loops_mutablet(it->second.body);
		loops[&(it->second.body)] = natural_loops;
	}
}

void acceleratort::accelerate_loop(goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	goto_programt dup_body;
	dup_body.copy_from(functions);
	Forall_goto_program_instructions(it, dup_body)
	{
		if (it->is_assert()) it->type = ASSUME;
	}

	goto_programt::targett sink = dup_body.add_instruction(ASSUME);
	sink->guard = false_exprt();
	goto_programt::targett end = dup_body.add_instruction(SKIP);

	goto_programt::targett start = dup_body.instructions.begin();
	for (auto tgt_it = functions.instructions.begin(), tgt_end =
			functions.instructions.end(); tgt_it != tgt_end;
			tgt_it++, start++) {
		if (loop.find(tgt_it) == loop.end()) {
			start->make_skip();
		}
		else if (tgt_it->is_goto()) {
			for (auto &t : tgt_it->targets) {
				if (t->location_number > tgt_it->location_number) {
					if (loop.find(t) == loop.end()) {
						start->targets.clear();
						start->targets.push_back(sink);
					}
				}
				else if (t == loop_header) {
					start->targets.clear();
					start->targets.push_back(end);
				}
				else {
					start->targets.clear();
					start->targets.push_back(sink);
				}
			}
		}
	}
	remove_skip(dup_body);
//	cout << "After\n==============================\n";
//	dup_body.output(cout);
}

void acceleratort::accelerate_all_loops(goto_programt &goto_function) {
	for (auto &loop : loops[&goto_function]->loop_map) {
		auto t = loop.first;
		accelerate_loop(t, loop.second, goto_function);
	}
}

void acceleratort::accelerate_all_functions() {
	Forall_goto_functions(it, goto_model.goto_functions)
	{
		accelerate_all_loops(it->second.body);
	}
}

bool acceleratort::accelerate() {
	register_language(new_ansi_c_language);
	register_language(new_cpp_language);
	cmdlinet c;
	std::string program = "";
	ui_message_handlert a(c, program);
	remove_function_pointers(a, goto_model, false);
	remove_virtual_functions(goto_model);
	remove_asm(goto_model);
	goto_inline(goto_model, a);
	remove_calls_no_bodyt remove_calls_no_body;
	remove_calls_no_body(goto_model.goto_functions);
	get_loops();
	accelerate_all_functions();
	return false;
}
