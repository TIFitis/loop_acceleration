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

void acceleratort::generate_paths(goto_programt &dup_body,
		goto_programt::targetst &branches,
		goto_programt::targetst::iterator cur_branch,
		map<goto_programt::targett, unsigned> path_explored,
		set<goto_programt*> &paths) {
	if (cur_branch == branches.end()) {
		goto_programt &path = *(new goto_programt());
		path.copy_from(dup_body);
		goto_programt::targett path_it = path.instructions.begin();
		for (auto tgt_it = dup_body.instructions.begin(), tgt_end =
				dup_body.instructions.end(); tgt_it != tgt_end;
				tgt_it++, path_it++) {
			if (tgt_it->is_goto()) {
				if (find(branches.begin(), branches.end(), tgt_it)
						== branches.end()) continue;
				if (tgt_it->guard == true_exprt()) {
					for (auto it_new = path_it, new_tgt_end =
							path_it->get_target(); it_new != new_tgt_end;
							it_new++, path_it++, tgt_it++) {
						it_new->make_skip();
					}
					path_it--;
					tgt_it--;
					continue;
				}
				if (path_explored[tgt_it] == 1) {
					auto branch_assum = path.insert_before(path_it);
					branch_assum->make_assumption(tgt_it->guard);
					for (auto it_new = path_it, new_it_end =
							path_it->get_target(); it_new != new_it_end;
							it_new++, tgt_it++, path_it++) {
						it_new->make_skip();
					}
					path_it--;
					tgt_it--;
					continue;
				}
				else if (path_explored[tgt_it] == 2) {
					auto branch_assum = path.insert_before(path_it);
					branch_assum->make_assumption(not_exprt(tgt_it->guard));
					path_it->make_skip();
					continue;
				}
			}
		}
		path.update();
		remove_skip(path);
		paths.insert(&path);
		return;
	}
	path_explored[*cur_branch] = 1;
	cur_branch++;
	generate_paths(dup_body, branches, cur_branch, path_explored, paths);
	cur_branch--;
	path_explored[*cur_branch] = 2;
	cur_branch++;
	generate_paths(dup_body, branches, cur_branch, path_explored, paths);
}

set<goto_programt*>& acceleratort::create_dup_loop(const goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	goto_programt &dup_body = *(new goto_programt());
	dup_body.copy_from(functions);
//	Forall_goto_program_instructions(it, dup_body)
//	{
//		if (it->is_assert()) it->type = ASSUME;
//	}

	goto_programt::targett sink = dup_body.add_instruction(ASSUME);
	sink->guard = false_exprt();
//	goto_programt::targett end = dup_body.add_instruction(SKIP);
//	auto end = (*loop.rbegin())->get_target();
	goto_programt::targett dup_body_it = dup_body.instructions.begin();
	goto_programt::targetst branches;
	for (auto tgt_it = functions.instructions.begin(), tgt_end =
			functions.instructions.end(); tgt_it != tgt_end;
			tgt_it++, dup_body_it++) {
		if (loop.find(tgt_it) == loop.end()) {
			dup_body_it->make_skip();
		}
		else if (tgt_it->is_goto()) {
			for (auto &t : tgt_it->targets) {
				if (t->location_number > tgt_it->location_number) {
					if (loop.find(t) == loop.end()) {
						dup_body_it->targets.clear();
						dup_body_it->targets.push_back(sink);
					}
					else {
						branches.push_back(dup_body_it);
					}
				}
				else if (t == loop_header) {
//					dup_body_it->make_skip();
//					dup_body_it->targets.clear();
//					dup_body_it->targets.push_back(end);
				}
				else {
					dup_body_it->targets.clear();
					dup_body_it->targets.push_back(sink);
				}
			}
		}
	}
	remove_skip(dup_body);
	exprst loop_vars;
	for (auto a : dup_body.instructions) {
		if (a.is_assign()) {
			gather_syms(a.code.op0(), loop_vars);
		}
	}
	set<goto_programt*> &paths = *new set<goto_programt*>();
	for (auto a : branches) {
		exprst temp;
		gather_syms(a->guard, temp);
		for (auto b : temp) {
			if (loop_vars.find(b) != loop_vars.end()) {
				cout << "LoopBody has non_invariant loop_cond" << endl;
				paths.clear();
				return paths;
			}
		}
	}

	map<goto_programt::targett, unsigned> path_explored;
#if DBGLEVEL >= 3
	if(!branches.empty())
		cout << "========branches==========" << endl;
	for (auto s : branches) {
		path_explored[s] = 0;
		cout << from_expr(s->guard) << endl;
	}
	if(!branches.empty())
		cout << "========branches==========" << endl;
#endif
	generate_paths(dup_body, branches, branches.begin(), path_explored, paths);
	map<string, goto_programt*> rem_duplicates;
	for (auto a : paths) {
		string s;
		stringstream *temp = new stringstream(s);
		temp->str();
		a->output(*temp);
		if (rem_duplicates.find(temp->str()) != rem_duplicates.end()) {
			delete a;
		}
		else
			rem_duplicates[temp->str()] = a;
	}
	paths.clear();
	for (auto &a : rem_duplicates) {
		paths.insert(a.second);
	}
#if DBGLEVEL >= 2
	cout << paths.size() << " paths generated!" << endl;
#endif
#if DBGLEVEL >= 5
	for (auto a : paths) {
		cout << "========path-begin==========" << endl;
		a->output(cout);
		cout << "========path-end==========" << endl;
	}
#endif
	return paths;
}

exprst acceleratort::gather_syms(exprt expr, exprst &expr_syms) {
	if (expr.id() == ID_symbol)
		expr_syms.insert(expr);
	else if (expr.id() == ID_index || expr.id() == ID_member
			|| expr.id() == ID_dereference)
		assert(false && "Not Handled type");
	else {
		forall_operands(it, expr) {
			gather_syms(*it, expr_syms);
		}
	}
	return expr_syms;
}

void acceleratort::get_all_sources(exprt tgt,
		goto_programt::instructionst &assign_insts,
		exprst &src_syms,
		exprst &visited_syms) {
	for (auto it = assign_insts.begin(); it != assign_insts.end(); ++it) {
		auto assignment = to_code_assign(it->code);
		if (assignment.lhs() == tgt) {
			gather_syms(assignment.rhs(), src_syms);
			for (auto op : src_syms) {
				if (visited_syms.find(op) == visited_syms.end()) {
					visited_syms.insert(op);
					get_all_sources(op, assign_insts, src_syms, visited_syms);
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
		symbol.name = "__acc_" + name + "_" + to_string(var_counter);
	symbol.base_name = symbol.name;
	assert(var_counter < UINT_MAX && "Oops too many variables have added !");
	var_counter++;
	symbol.type = type;
	symbol.mode = ID_C;
	return symbol;
}

bool acceleratort::check_pattern(code_assignt &inst_c, exprt n_e) {
	auto l_s = to_symbol_expr(inst_c.lhs());
	auto r_e = inst_c.rhs();
	if (can_cast_expr<plus_exprt>(r_e)) {
		auto e0 = r_e.op0();
		auto e1 = r_e.op1();
		if (can_cast_expr<symbol_exprt>(e0)) {
			auto r_s = to_symbol_expr(e0);
			if (r_s.get_identifier() == l_s.get_identifier()) {
				if (can_cast_expr<constant_exprt>(e1)
						|| can_cast_expr<symbol_exprt>(e1)) {
					auto mod_expr = plus_exprt(e0, mult_exprt(e1, n_e));
					inst_c.rhs() = mod_expr;
					inst_c = code_assignt(inst_c.lhs(), mod_expr);
					return true;
				}
				else
					return false;
			}
			else
				return false;
		}
	}
	return false;
}

void add_overflow_assumes(goto_programt &g_p,
		goto_programt::targett &loc,
		exprt rh) {
	if (rh.operands().size() >= 1) for (auto op : rh.operands())
		add_overflow_assumes(g_p, loc, op);
	auto bl = g_p.insert_after(loc);
	bl->make_skip();
	loc = bl;
	mp_integer max;
	mp_integer min;
	if (can_cast_type<unsignedbv_typet>(rh.type())) {
		max = to_unsignedbv_type(rh.type()).largest();
		min = to_unsignedbv_type(rh.type()).smallest();
	}
	else if (can_cast_type<signedbv_typet>(rh.type())) {
		max = to_signedbv_type(rh.type()).largest();
		min = to_signedbv_type(rh.type()).smallest();
	}
	else if (rh.type().id() == ID_bool)
		return;
	else {
		cerr << from_expr(rh) << endl;
		cerr << rh.type().id().c_str() << endl;
		assert(false && "Unhandled overflow type!");
	}

	if (rh.operands().size() > 1) {
		if (can_cast_expr<mult_exprt>(rh)) {
			bl->make_assumption(binary_relation_exprt(rh.op0(),
					ID_lt,
					div_exprt(from_integer(max, rh.type()), rh.op1())));
			bl = g_p.insert_after(bl);
			bl->make_assumption(binary_relation_exprt(rh.op0(),
					ID_gt,
					div_exprt(from_integer(min, rh.type()), rh.op1())));
			loc = bl;
		}
		else {
			if (can_cast_expr<minus_exprt>(rh))
				rh = plus_exprt(rh.op0(), unary_minus_exprt(rh.op1()));
			if (can_cast_expr<plus_exprt>(rh)) {
				if (can_cast_type<unsignedbv_typet>(rh.op0().type())
						&& can_cast_type<unsignedbv_typet>(rh.op1().type()))
					bl->make_assumption(and_exprt(binary_relation_exprt(rh,
							ID_gt,
							rh.op1()),
							(binary_relation_exprt(rh, ID_gt, rh.op0()))));
				else if (can_cast_type<signedbv_typet>(rh.op0().type())
						&& can_cast_type<signedbv_typet>(rh.op1().type())) {
					auto zero_expr = from_integer(0, rh.type());
					bl->make_assumption(and_exprt(implies_exprt(and_exprt(binary_relation_exprt(rh.op0(),
							ID_gt,
							zero_expr),
							(binary_relation_exprt(rh.op1(), ID_gt, zero_expr))),
							binary_relation_exprt(rh, ID_gt, zero_expr)),
							implies_exprt(and_exprt(binary_relation_exprt(rh.op0(),
									ID_lt,
									zero_expr),
									(binary_relation_exprt(rh.op1(),
											ID_lt,
											zero_expr))),
									binary_relation_exprt(rh,
											ID_lt,
											zero_expr))));
				}
				else {
					cerr << from_expr(rh) << endl;
					cerr << rh.type().id().c_str() << endl;
					assert(false && "Unhandled overflow type!");
				}
			}
			else {
				bl->make_assumption(binary_relation_exprt(rh,
						ID_lt,
						from_integer(max, rh.type())));
				bl = g_p.insert_after(bl);
				bl->make_assumption(binary_relation_exprt(rh,
						ID_gt,
						from_integer(min, rh.type())));
				loc = bl;
			}
		}
	}
}

void acceleratort::add_overflow_checks(goto_programt &g_p) {
	for (auto start = g_p.instructions.begin(), end = g_p.instructions.end();
			start != end; start++) {
		if (start->is_assume()) {
			auto rh = start->guard;
			auto hoist_loc = g_p.insert_before(start);
			hoist_loc->make_skip();
//			add_overflow_assumes(g_p, hoist_loc, rh);
		} // for loop
		if (start->is_assign()) {
			auto x = to_code_assign(start->code);
			auto rh = x.rhs();
			auto hoist_loc = g_p.insert_before(start);
			hoist_loc->make_skip();
//			add_overflow_assumes(g_p, hoist_loc, rh);
		}
	}
	remove_skip(g_p);
	g_p.update();
}

bool acceleratort::augment_path(goto_programt::targett &loop_header,
		goto_programt &functions,
		goto_programt &aux_path) {
#if DBGLEVEL >= 3
	cout << "========auxpath==========" << endl;
	aux_path.output(cout);
	cout << "========auxpath==========" << endl;
#endif
	auto split = functions.insert_before(loop_header);
	split->make_goto(loop_header,
			side_effect_expr_nondett(bool_typet(), split->source_location));
	split->code = code_gotot();
	functions.destructive_insert(loop_header, aux_path);
	functions.update();
	loop_header = split;
	return true;
}

void acceleratort::swap_all(exprt &l_c, const exprt &n_e, const exprt &j_e) {
	for (auto &op : l_c.operands()) {
		swap_all(op, n_e, j_e);
		if (op == n_e) {
			op = j_e;
		}
	}
	if (l_c == n_e) {
		l_c = j_e;
	}
}

void acceleratort::precondition(goto_programt &g_p,
		goto_programt::targett loc,
		goto_programt::targett sink,
		exprt loop_cond) {
//	auto n_exp = goto_model.symbol_table.lookup(ACC_N)->symbol_expr();
//	auto j_asgn = g_p.insert_after(loc);
//	auto j_exp = goto_model.symbol_table.lookup(ACC_J)->symbol_expr();
//	j_asgn->make_assignment();
//	j_asgn->code = code_assignt(j_exp,
//			side_effect_expr_nondett(j_exp.type(), j_asgn->source_location));
//	auto j_ge_0 = g_p.insert_after(j_asgn);
//	j_ge_0->make_assumption(binary_relation_exprt(j_exp,
//			ID_ge,
//			from_integer(0, j_exp.type())));
//	auto j_lt_n = g_p.insert_after(j_ge_0);
//	j_lt_n->make_assumption(binary_relation_exprt(j_exp, ID_lt, n_exp));
//	auto forall_assump = g_p.insert_after(j_lt_n);
//	forall_assump->make_assumption(forall_exprt(j_exp,
//			and_exprt(and_exprt(binary_relation_exprt(j_exp,
//					ID_ge,
//					from_integer(0, j_exp.type())),
//					binary_relation_exprt(j_exp, ID_le, n_exp)),
//					loop_cond)));
	auto forall_assump2 = g_p.insert_after(loc);
	forall_assump2->make_assumption(loop_cond);
}

bool acceleratort::syntactic_matching(goto_programt &g_p,
		goto_programt::instructionst &assign_insts,
		exprt loop_cond,
		goto_programt::targett sink) {
#if NO_SYNTACTIC
	return false;
#endif
	goto_programt g_p_c;
	g_p_c.copy_from(g_p);
	goto_programt::instructionst assign_insts_c = assign_insts;
	auto n_exp = goto_model.symbol_table.lookup(ACC_N)->symbol_expr();
	auto j_exp = goto_model.symbol_table.lookup(ACC_J)->symbol_expr();
	for (auto &inst : assign_insts_c) {
		g_p_c.update();
		auto &inst_code = to_code_assign(inst.code);
		auto tgt = inst_code.lhs();
		exprst src_syms, visited_syms;
		get_all_sources(tgt, assign_insts_c, src_syms, visited_syms);
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
	swap_all(loop_cond,
			n_exp,
			minus_exprt(n_exp, from_integer(1, n_exp.type())));
	goto_programt::targett si = g_p.instructions.begin();
	for (auto it = g_p.instructions.begin(), it_e = g_p.instructions.end();
			it_e != it;) {
		it++;
		if (it != it_e) si++;
	}
	goto_programt::targett hoist_loc =
			g_p.insert_before(g_p.instructions.begin());
	hoist_loc->make_skip();
	precondition(g_p, hoist_loc, si, loop_cond);
	g_p.update();
	add_overflow_checks(g_p);
	return true;
}

void sanitize_types(exprt &expr) {
	if (expr.operands().size() > 1) {
//		for (auto e : expr.operands()) {
		sanitize_types(expr.op0());
		sanitize_types(expr.op1());
		expr.op0() = typecast_exprt::conditional_cast(expr.op0(),
				expr.op1().type());
		expr.type() = expr.op1().type();
//		}
	}
}

bool acceleratort::constraint_solver(goto_programt &g_p,
		goto_programt::instructionst &assign_insts,
		exprt &loop_cond,
		goto_programt::targett sink) {
	goto_programt g_p_c;
	g_p_c.copy_from(g_p);
	goto_programt::instructionst assign_insts_c = assign_insts;
	exprt n_exp = goto_model.symbol_table.lookup(ACC_N)->symbol_expr();
//	exprt j_exp = goto_model.symbol_table.lookup(ACC_J)->symbol_expr();
	exprst loop_vars;
	for (auto inst : assign_insts_c) {
		gather_syms(inst.code.op0(), loop_vars);
		gather_syms(inst.code.op1(), loop_vars);
	}
	map<exprt, exprt> last_asgn;
	map<exprt, exprt> hoist_tgt;
	goto_programt::targett hoist_loc =
			g_p_c.insert_before(g_p_c.instructions.begin());
	hoist_loc->make_skip();
	unsigned new_inst_added = 1;
	for (auto it = assign_insts_c.begin(), it_e = assign_insts_c.end();
			it != it_e; it++) {
		g_p_c.update();
		auto inst = *it;
		auto &inst_code = to_code_assign(inst.code);
		auto tgt = inst_code.lhs();
		if (hoist_tgt.find(tgt) == hoist_tgt.end()) {
			auto sym = create_symbol(from_expr(tgt), tgt.type());
			goto_model.symbol_table.insert(sym);
			auto hoist_inst = g_p_c.insert_after(hoist_loc);
			hoist_inst->make_assignment();
			hoist_inst->code = code_assignt(sym.symbol_expr(), tgt);
			hoist_tgt[tgt] = sym.symbol_expr();
			hoist_loc = hoist_inst;
			new_inst_added++;
		}
		exprst src_syms, visited_syms;
		get_all_sources(tgt, assign_insts_c, src_syms, visited_syms);
		if (find(src_syms.begin(), src_syms.end(), tgt) == src_syms.end()) {
			continue;
		}
		z3_parse parser { };
		parser.new_build(loop_vars, tgt, assign_insts_c, inst);
		parser.z3_fire();
		auto z3_model = parser.get_z3_model("z3_results.dat");
		if (z3_model.empty()) {
			cout << "Acceleration failed for : "
					<< from_expr(inst.code)
					<< endl;
			return false;
		}
		exprt accelerated_func = parser.getAccFunc(z3_model, loop_vars, n_exp);

#if DBGLEVEL >= 3
		cout << "\n\nClosedForm expr:\n" << from_expr(accelerated_func) << endl;
#endif
		simplify(accelerated_func, namespacet(goto_model.symbol_table));
#if DBGLEVEL >= 3
		cout << "\n\nSimplified expr:\n" << from_expr(accelerated_func) << endl;
#endif
//		swap_all(accelerated_func, tgt, hoist_tgt[tgt]);
		sanitize_types(accelerated_func);
		accelerated_func = typecast_exprt::conditional_cast(accelerated_func,
				inst_code.lhs().type());
		inst_code.rhs() = accelerated_func;
		last_asgn[tgt] = inst_code.rhs();
		auto x = g_p_c.instructions.begin();
		advance(x, inst.location_number + new_inst_added);
		x->code = inst_code;
		g_p_c.update();
	}
	for (auto &i : g_p_c.instructions) {
		if (i.is_assign()) {
			for (auto t : hoist_tgt) {
				if (i.code.op0() != t.second)
					swap_all(i.code.op1(), t.first, t.second);

			}
		}
	}
	for (auto a : last_asgn) {
		swap_all(loop_cond, a.first, a.second);
	}
	exprt loop_cond_dup = loop_cond;
	swap_all(loop_cond_dup,
			n_exp,
			minus_exprt(n_exp, from_integer(1, n_exp.type())));
	goto_programt::targett si = g_p_c.instructions.begin();
	for (auto it = g_p_c.instructions.begin(), it_e = g_p_c.instructions.end();
			it_e != it;) {
		it++;
		if (it != it_e) si++;
	}
	precondition(g_p_c, hoist_loc, si, loop_cond_dup);
	g_p_c.update();
	g_p.copy_from(g_p_c);
	add_overflow_checks(g_p);
	g_p.update();
	assign_insts = assign_insts_c;
	return true;
}

void fix_goto_exit(goto_programt &g_p, const goto_programt::targett &end) {
	goto_programt::targett path_loop_header;
	for (auto it = g_p.instructions.begin();; it++)
		if (it->is_goto()) {
			path_loop_header = it;
			break;
		}
	for (auto i = g_p.instructions.begin(), e = g_p.instructions.end(); i != e;
			i++) {
		if (i->is_goto()) {
			for (auto &t : i->targets) {
				if (t == path_loop_header) {
					i->targets.clear();
					i->targets.push_back(end);
					break;
				}
			}
		}
	}
}

void acceleratort::accelerate_loop(const goto_programt::targett &loop_header,
		natural_loops_mutablet::natural_loopt &loop,
		goto_programt &functions) {
	auto &paths = create_dup_loop(loop_header, loop, functions);
	exprt loop_cond_o;
	if (loop_header->is_goto()) {
		if (can_cast_expr<not_exprt>(loop_header->guard))
			loop_cond_o = to_not_expr(loop_header->guard).op0();
		else
			loop_cond_o = not_exprt(loop_header->guard);
	}
	auto split_loc = loop_header;
	auto n_sym = create_symbol(ACC_N, unsignedbv_typet(32), true);
	goto_model.symbol_table.insert(n_sym);
	auto n_exp = n_sym.symbol_expr();
//	auto j_sym = create_symbol(ACC_J, unsignedbv_typet(32), true);
//	goto_model.symbol_table.insert(j_sym);
//	auto j_exp = j_sym.symbol_expr();
//	auto p_sym = create_symbol(ACC_P, bool_typet(), true);
//	goto_model.symbol_table.insert(p_sym);
	auto end = loop_header->get_target();
	unsigned no_paths_accelerated = 0;
	for (auto path_ptr : paths) {

		exprt loop_cond = loop_cond_o;
		auto &path = *path_ptr;
		auto path_loop_header = path.instructions.begin();
		auto sink = path.instructions.end();
		sink--;
		assert(sink->is_assume() && sink->guard == false_exprt()
				&& "Sink not found !!!");
		auto dup_body_iter = path.instructions.begin();
		goto_programt::instructionst assign_insts;
		exprst assign_tgts;
		for (auto inst_it = path.instructions.begin(), inst_end =
				path.instructions.end(); inst_it != inst_end;
				inst_it++, dup_body_iter++) {
			if (inst_it->is_assign()) {
				assign_insts.push_back(*inst_it);
				auto &x = to_code_assign(inst_it->code);
				assign_tgts.insert(x.lhs());
			}
		}
//		auto safe = path.insert_after(sink);
//		safe->make_skip();
//		auto goto_safe = path.insert_before(sink);
//		goto_safe->make_goto(safe, true_exprt());

		if (syntactic_matching(path, assign_insts, loop_cond, sink)) {
#if DBGLEVEL >= 3
			cout << "SyntactingMatching accelerated :: " << endl;
#endif
			no_paths_accelerated++;
			for (auto it = path.instructions.begin();; it++)
				if (it->is_goto()) {
					path_loop_header = it;
					break;
				}
			for (auto &i : path.instructions) {
				if (i.is_goto()) {
					for (auto &t : i.targets) {
						if (t == path_loop_header) {
							i.targets.clear();
							i.targets.push_back(end);
							break;
						}
					}
				}
			}
			augment_path(split_loc, functions, path);
		}
		else if (constraint_solver(path, assign_insts, loop_cond, sink)) {
#if DBGLEVEL >= 3
			cout << "ConstraintSolving accelerated :: " << endl;
#endif
			no_paths_accelerated++;
			auto n_asgn = path.insert_before(path.instructions.begin());
			n_asgn->make_assignment();
			n_asgn->code = code_assignt(n_exp,
					side_effect_expr_nondett(n_exp.type(),
							n_asgn->source_location));
			auto n_ge_1 = path.insert_after(n_asgn);
			n_ge_1->make_assumption(binary_relation_exprt(n_exp,
					ID_ge,
					from_integer(1, n_exp.type())));
			path.update();
			goto_programt no_post_path;
			no_post_path.copy_from(path);
			auto false_sink = no_post_path.add_instruction();
			false_sink->make_assumption(false_exprt());
			no_post_path.update();
			fix_goto_exit(no_post_path, false_sink);
			augment_path(split_loc, functions, no_post_path);

			for (auto it = path.instructions.begin();; it++)
				if (it->is_goto()) {
					path_loop_header = it;
					break;
				}
			auto post_cond = path.insert_before(path_loop_header);
			post_cond->make_assumption(not_exprt(loop_cond));
			path.update();
			fix_goto_exit(path, end);
			augment_path(split_loc, functions, path);
		}
	}
	cout << "Accelerated " << no_paths_accelerated << " paths." << endl;
//	auto n_asgn = functions.insert_before(functions.instructions.begin());
//	n_asgn->make_assignment();
//	n_asgn->code = code_assignt(n_exp,
//			side_effect_expr_nondett(n_exp.type(), n_asgn->source_location));
//	auto n_ge_1 = functions.insert_after(n_asgn);
//	n_ge_1->make_assumption(binary_relation_exprt(n_exp,
//			ID_ge,
//			from_integer(1, n_exp.type())));
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

void acceleratort::sanitize_overflow() {
	Forall_goto_functions(it, goto_model.goto_functions)
	{
		for (auto &a : it->second.body.instructions) {
			if (a.is_assert()) {
				auto g = a.guard;
				string str = from_expr(g);
				if (str.find("!overflow") != str.npos) {
					cout << from_expr(g) << "is OVERFLOW" << endl;
					a.make_skip();
					a.make_assumption(g);
				}
				else {
					cout << from_expr(g) << "ISNT OVERFLOW" << endl;
				}
			}
		}
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
	optionst options;
	goto_model.goto_functions.update();
	options.set_option("signed-overflow-check", 1);
	options.set_option("unsigned-overflow-check", 1);
	goto_check(options, goto_model);
	sanitize_overflow();
	return false;
}
