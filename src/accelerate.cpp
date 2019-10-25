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

goto_programt& acceleratort::create_dup_loop(goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	goto_programt &dup_body = *(new goto_programt());
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
	return dup_body;
}

expr_sett acceleratort::gather_syms(exprt expr) {
	expr_sett expr_syms;
	if (expr.id() == ID_symbol)
		expr_syms.insert(expr);
	else if (expr.id() == ID_index || expr.id() == ID_member
			|| expr.id() == ID_dereference)
		assert(false && "Not Handled type");
	else {
		forall_operands(it, expr) {
			for (auto a : gather_syms(*it))
				expr_syms.insert(a);
		}
	}
	return expr_syms;
}

void acceleratort::get_all_sources(exprt tgt,
		goto_programt::instructionst &assign_insts,
		expr_sett &src_syms,
		goto_programt::instructionst &clustered_asgn_insts) {
	src_syms = gather_syms(tgt);

	for (goto_programt::instructionst::reverse_iterator r_it =
			assign_insts.rbegin(); r_it != assign_insts.rend(); ++r_it) {
		if (r_it->is_assign()) {
			auto assignment = to_code_assign(r_it->code);
			auto lhs_syms = gather_syms(assignment.lhs());

			if (assignment.lhs().id() == ID_symbol) src_syms.insert(tgt);

			for (auto s_it : lhs_syms) {
				if (find(src_syms.begin(), src_syms.end(), s_it)
						!= src_syms.end()) {
					clustered_asgn_insts.push_front(*r_it);
					src_syms.erase(assignment.lhs());
					for (auto a : gather_syms(assignment.rhs()))
						src_syms.insert(a);
					break;
				}
			}
		}
	}
}

void acceleratort::accelerate_loop(goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	auto &dup_body = create_dup_loop(loop_header, loop, functions);
	auto dup_body_iter = dup_body.instructions.begin();
//	cout << "After\n==============================\n";
//	dup_body.output(cout);
	goto_programt::instructionst assign_insts;
	expr_sett assign_tgts;
	for (auto inst_it = dup_body.instructions.begin(), inst_end =
			dup_body.instructions.end(); inst_it != inst_end;
			inst_it++, dup_body_iter++) {
		if (inst_it->is_assign()) {
			assign_insts.push_back(*inst_it);
			auto &x = to_code_assign(inst_it->code);
			assign_tgts.insert(x.lhs());
		}
	}
	expr_sett non_recursive_tgts;
	for (auto tgt : assign_tgts) {
		cout << "doing stuff for :: " << from_expr(tgt) << endl << endl;
		expr_sett src_syms;
		goto_programt::instructionst clustered_asgn_insts;
		get_all_sources(tgt, assign_insts, src_syms, clustered_asgn_insts);
		cout << "target_syms : " << endl;
		for (auto a : src_syms)
			cout << from_expr(a) << ", ";
		cout << "\n sliced_assign_insts : " << endl;
		for (auto a : clustered_asgn_insts)
			cout << from_expr(a.code) << endl;
		if (find(src_syms.begin(), src_syms.end(), tgt) == src_syms.end()) {
			non_recursive_tgts.insert(tgt);
			continue;
		}
		else {

		}

	}
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
