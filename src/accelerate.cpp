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
		goto_programt &functions,
		goto_programt::targett &loop_sink) {
	goto_programt &dup_body = *(new goto_programt());
	dup_body.copy_from(functions);
//	Forall_goto_program_instructions(it, dup_body)
//	{
//		if (it->is_assert()) it->type = ASSUME;
//	}

	goto_programt::targett sink = dup_body.add_instruction(ASSUME);
	sink->guard = false_exprt();
	loop_sink = sink;
//	goto_programt::targett end = dup_body.add_instruction(SKIP);
//	auto end = (*loop.rbegin())->get_target();
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
					start->make_skip();
//					start->targets.clear();
//					start->targets.push_back(end);
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

symbolt acceleratort::create_symbol(string name,
		const typet &type,
		bool force) {
	symbolt symbol;
	static unsigned var_counter;
	symbol.is_file_local = true;
	symbol.is_thread_local = true;
	symbol.is_lvalue = true;

	if (force)
		symbol.name = name;
	else
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

bool acceleratort::check_pattern(code_assignt &inst_c, exprt n_e) {
	auto l_e = to_symbol_expr(inst_c.lhs());
	auto r_e = inst_c.rhs();
	if (can_cast_expr<symbol_exprt>(r_e)) {
		auto r_s = to_symbol_expr(r_e);
		return true;
	}
	else if (can_cast_expr<plus_exprt>(r_e)) {
		auto e0 = r_e.op0();
		auto e1 = r_e.op1();
		if (can_cast_expr<symbol_exprt>(e0)) {
			auto r_s = to_symbol_expr(e0);
			if (r_s.get_identifier() == l_e.get_identifier()) {
				if (can_cast_expr<plus_exprt>(e1)) {

				}
				else {
					auto mod_expr = plus_exprt(e0, mult_exprt(e1, n_e));
					inst_c.rhs() = mod_expr;
					inst_c = code_assignt(inst_c.lhs(), mod_expr);
					return true;
				}
			}
			else
				return false;
		}
	}
	return false;
}

bool acceleratort::augment_path(goto_programt::targett &loop_header,
		goto_programt &functions,
		goto_programt &aux_path) {
	cout << "========auxpath==========" << endl;
	aux_path.output(cout);
	cout << "========auxpath==========" << endl;
	auto split = functions.insert_before(loop_header);
	split->make_goto(loop_header,
			side_effect_expr_nondett(bool_typet(), split->source_location));
	split->code = code_gotot();
	functions.update();
	functions.destructive_insert(loop_header, aux_path);
	functions.update();
	return true;
}

void swap_all(exprt &l_c, exprt &n_e, exprt &j_e) {
	for (auto &op : l_c.operands()) {
		if (op == n_e) {
			op = j_e;
		}
		swap_all(op, n_e, j_e);
	}
}

void acceleratort::precondition(goto_programt &g_p,
		goto_programt::targett loc,
		goto_programt::targett sink,
		exprt loop_cond) {
	auto n_exp = goto_model.symbol_table.lookup(ACC_N)->symbol_expr();
	auto j_exp = goto_model.symbol_table.lookup(ACC_J)->symbol_expr();
	auto n_asgn = g_p.insert_before(loc);
	n_asgn->make_assignment();
	n_asgn->code = code_assignt(n_exp,
			side_effect_expr_nondett(n_exp.type(), n_asgn->source_location));
	auto n_ge_1 = g_p.insert_after(n_asgn);
	n_ge_1->make_assumption(binary_relation_exprt(n_exp,
			ID_ge,
			from_integer(1, n_exp.type())));
	auto j_asgn = g_p.insert_after(n_ge_1);
	j_asgn->make_assignment();
	j_asgn->code = code_assignt(j_exp,
			side_effect_expr_nondett(j_exp.type(), j_asgn->source_location));
	auto j_ge_0 = g_p.insert_after(j_asgn);
	j_ge_0->make_assumption(binary_relation_exprt(j_exp,
			ID_ge,
			from_integer(0, j_exp.type())));
	auto j_lt_n = g_p.insert_after(j_ge_0);
	j_lt_n->make_assumption(binary_relation_exprt(j_exp, ID_lt, n_exp));
	auto forall_assump = g_p.insert_after(j_lt_n);
	forall_assump->make_goto(sink,
			forall_exprt(j_exp,
					and_exprt(and_exprt(binary_relation_exprt(j_exp,
							ID_ge,
							from_integer(0, j_exp.type())),
							binary_relation_exprt(j_exp, ID_le, n_exp)),
							loop_cond)));
}

bool acceleratort::syntactic_matching(goto_programt &g_p,
		goto_programt::instructionst &assign_insts,
		exprt loop_cond,
		goto_programt::targett sink) {
	goto_programt g_p_c;
	g_p_c.copy_from(g_p);
	goto_programt::instructionst assign_insts_c = assign_insts;
	auto n_exp = goto_model.symbol_table.lookup(ACC_N)->symbol_expr();
	auto j_exp = goto_model.symbol_table.lookup(ACC_J)->symbol_expr();
	for (auto &inst : assign_insts_c) {
		g_p_c.update();
		auto &inst_code = to_code_assign(inst.code);
		auto tgt = inst_code.lhs();
		exprst src_syms;
		goto_programt::instructionst clustered_asgn_insts;
		get_all_sources(tgt, assign_insts_c, src_syms, clustered_asgn_insts);
		if (find(src_syms.begin(), src_syms.end(), tgt) == src_syms.end()) {
			continue;
		}
		if (!check_pattern(inst_code, n_exp)) {
			return false;
		}
		swap_all(loop_cond, tgt, inst_code.rhs());
		auto x = g_p_c.instructions.begin();
		advance(x, inst.location_number);
		x->code = inst_code;
		g_p_c.update();
	}
	g_p.copy_from(g_p_c);
	g_p.update();
	assign_insts = assign_insts_c;
	swap_all(loop_cond, n_exp, j_exp);
	goto_programt::targett si = g_p.instructions.begin();
	for (auto x = g_p.instructions.begin(), e = g_p.instructions.end(); e != x;
			) {
		x++;
		if (x != e) si++;
	}
	precondition(g_p, g_p.instructions.begin(), si, loop_cond);
	g_p.update();
	return true;
}

//bool constraint_accelerator() {
//
//}

void acceleratort::accelerate_loop(goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	goto_programt::targett sink;
	auto &dup_body = create_dup_loop(loop_header, loop, functions, sink);
	auto dup_body_iter = dup_body.instructions.begin();
	exprt loop_cond;
	if (loop_header->is_goto()) {
		loop_cond = not_exprt(loop_header->guard);
	}
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
	auto n_sym = create_symbol(ACC_N, signedbv_typet(32), true);
	goto_model.symbol_table.insert(n_sym);
	auto n_exp = n_sym.symbol_expr();
	auto j_sym = create_symbol(ACC_J, signedbv_typet(32), true);
	goto_model.symbol_table.insert(j_sym);
	auto j_exp = j_sym.symbol_expr();

	if (syntactic_matching(dup_body, assign_insts, loop_cond, sink)) {
		cout << "Syntacting Matching accelerated :: " << endl;
		augment_path(loop_header, functions, dup_body);
		return;
	}
	else {
		for (auto tgt : assign_tgts) {
			symbol_exprt se = to_symbol_expr(tgt);
			exprst src_syms;
			goto_programt::instructionst clustered_asgn_insts;
			get_all_sources(tgt, assign_insts, src_syms, clustered_asgn_insts);
			cout << "src_syms : " << endl;
			if (find(src_syms.begin(), src_syms.end(), tgt) == src_syms.end()) {
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
				z3_parse parser { };
				goto_programt::instructionst tgt_asgn_insts;
				for (auto &inst : assign_insts) {
					auto &inst_code = to_code_assign(inst.code);
					if (inst_code.lhs() == tgt) tgt_asgn_insts.push_back(inst);
				}
				auto z3_formula = parser.buildFormula(inf,
						from_expr(tgt),
						tgt_asgn_insts);
				cout << z3_formula << endl;
				parser.z3_fire(z3_formula);
				auto z3_model = parser.get_z3_model("z3_results.dat");

				exprt accelerated_func = parser.getAccFunc(n_exp, z3_model);

				std::cout << "Made acc expr: "
						<< from_expr(accelerated_func)
						<< std::endl;
			}

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
//	remove_function_pointers(mh, goto_model, false);
//	remove_virtual_functions(goto_model);
//	remove_asm(goto_model);
//	goto_inline(goto_model, mh);
//	remove_calls_no_bodyt remove_calls_no_body;
//	remove_calls_no_body(goto_model.goto_functions);
	get_loops();
	accelerate_all_functions();
	return false;
}
