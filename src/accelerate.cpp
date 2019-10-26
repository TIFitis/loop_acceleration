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

exprst acceleratort::gather_syms(exprt expr) {
	exprst expr_syms;
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
		exprst &src_syms,
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

symbolt acceleratort::create_symbol(string name, const typet &type) {
	symbolt symbol;
	static unsigned var_counter;
	symbol.is_file_local = true;
	symbol.is_thread_local = true;
	symbol.is_lvalue = true;

	symbol.name = name + to_string(var_counter);
	symbol.base_name = symbol.name;
	var_counter++;
	symbol.type = type;
	symbol.mode = ID_C;
	return symbol;
}

typet join_types(const typet &t1, const typet &t2) {
	if (t1 == t2) {
		return t1;
	}
	if ((t1.id() == ID_signedbv || t1.id() == ID_unsignedbv)
			&& (t2.id() == ID_signedbv || t2.id() == ID_unsignedbv)) {

		bitvector_typet b1 = to_bitvector_type(t1);
		bitvector_typet b2 = to_bitvector_type(t2);

		if (b1.id() == ID_unsignedbv && b2.id() == ID_unsignedbv) {
			size_t width = max(b1.get_width(), b2.get_width());
			return unsignedbv_typet(width);
		}
		else if (b1.id() == ID_signedbv && b2.id() == ID_signedbv) {
			size_t width = max(b1.get_width(), b2.get_width());
			return signedbv_typet(width);
		}
		else {
			size_t signed_width =
					t1.id() == ID_signedbv ? b1.get_width() : b2.get_width();
			size_t unsigned_width =
					t1.id() == ID_signedbv ? b2.get_width() : b1.get_width();

			size_t width = max(signed_width, unsigned_width);

			return signedbv_typet(width);
		}
	}
	assert(false && "Unhandled join types");
}

map<string, int> acceleratort::get_z3_model(string filename) {
	FILE *fp;
	fp = fopen(filename.c_str(), "r");
	assert(fp != nullptr && "Could not open z3_results.dat file");
	char s[4096];
	string raw_input = "";
	while (fgets(s, 4096, fp))
		raw_input += s;
	map<string, int> values;
	while (1) {
		auto loc = raw_input.find("define-fun");
		if (loc == raw_input.npos) break;
		raw_input = raw_input.substr(loc + 11, raw_input.npos);
		loc = raw_input.find(" ");
		auto name = raw_input.substr(0, loc);
		loc = raw_input.find("Int");
		raw_input = raw_input.substr(loc + 8, raw_input.npos);
		loc = raw_input.find(")");
		auto val_s = raw_input.substr(0, loc);
		stringstream val_ss(val_s);
		int x = 0;
		val_ss >> x;
		values[name] = x;
	}
	for (auto a : values)
		cout << a.first << "::" << a.second << endl;
	return values;
}

bool acceleratort::z3_fire(const string &z3_formula) {
	FILE *fp = fopen("z3_input.smt", "w");
	assert(fp != nullptr && "couldnt create input file for z3");
	fputs(z3_formula.c_str(), fp);
	string z3_command = "z3 -smt2 z3_input.smt 2> z3_results.dat";
	system(z3_command.c_str());
	return false;
}

void acceleratort::accelerate_loop(goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	auto &dup_body = create_dup_loop(loop_header, loop, functions);
	auto dup_body_iter = dup_body.instructions.begin();
//	cout << "After\n==============================\n";
//	dup_body.output(cout);
	goto_programt::instructionst assign_insts;
	exprst assign_tgts;
	for (auto inst_it = dup_body.instructions.begin(), inst_end =
			dup_body.instructions.end(); inst_it != inst_end;
			inst_it++, dup_body_iter++) {
		if (inst_it->is_assign()) {
			assign_insts.push_back(*inst_it);
			auto &x = to_code_assign(inst_it->code);
			assign_tgts.insert(x.lhs());
		}
	}
	exprst non_recursive_tgts;
	for (auto tgt : assign_tgts) {
		symbol_exprt se = to_symbol_expr(tgt);
		cout << se.get_identifier().c_str() << endl;

		cout << "doing stuff for :: " << from_expr(tgt) << endl << endl;
		exprst src_syms;
		goto_programt::instructionst clustered_asgn_insts;
		get_all_sources(tgt, assign_insts, src_syms, clustered_asgn_insts);
		cout << "src_syms : " << endl;
		for (auto a : src_syms)
			cout << from_expr(a) << ", ";
		cout << "\n clustered_asgn_insts : " << endl;
		for (auto a : clustered_asgn_insts)
			cout << from_expr(a.code) << endl;
		if (find(src_syms.begin(), src_syms.end(), tgt) == src_syms.end()) {
			non_recursive_tgts.insert(tgt);
			continue;
		}
		else {
			//fit_polynomial_sliced(clustered_asgn_insts, tgt, src_syms);
			// NOTE! Here we expect src_syms for JUST the current variable we are dealing with.
			// TODO: Need to make a set of sets/map for src_sym of each variable, and pass it here.
			// Todo: Also add each instruction constraint to the z3_parser!
			std::set<exprt> inf;
			for (auto a : src_syms)
				inf.insert(a);
			std::string s("i");
			z3_parse parser { };
			auto z3_formula = parser.buildFormula(inf, from_expr(tgt));
			z3_fire(z3_formula);
			auto z3_model = get_z3_model("z3_results.dat");
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
	string program = "";
	ui_message_handlert mh(c, program);
	remove_function_pointers(mh, goto_model, false);
	remove_virtual_functions(goto_model);
	remove_asm(goto_model);
	goto_inline(goto_model, mh);
	remove_calls_no_bodyt remove_calls_no_body;
	remove_calls_no_body(goto_model.goto_functions);
	get_loops();
	accelerate_all_functions();
	return false;
}
