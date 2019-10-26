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

void acceleratort::assert_for_values(scratch_programt &program,
		map<exprt, int> &values,
		set<pair<list<exprt>, exprt> > &coefficients,
		int num_unwindings,
		goto_programt::instructionst &loop_body,
		exprt &target) {
	typet expr_type = nil_typet();
	auto loop_counter = nil_exprt();

	for (map<exprt, int>::iterator it = values.begin(); it != values.end();
			++it) {
		typet this_type = it->first.type();
		if (this_type.id() == ID_pointer) {
			this_type = size_type();
		}

		if (expr_type == nil_typet()) {
			expr_type = this_type;
		}
		else {
			expr_type = join_types(expr_type, this_type);
		}
	}
	assert(to_bitvector_type(expr_type).get_width() > 0);
	for (map<exprt, int>::iterator it = values.begin(); it != values.end();
			++it) {
		program.assign(it->first, from_integer(it->second, expr_type));
	}
	for (int i = 0; i < num_unwindings; i++) {
		program.append(loop_body);
	}
	exprt rhs = nil_exprt();

	for (set<pair<list<exprt>, exprt> >::iterator it = coefficients.begin();
			it != coefficients.end(); ++it) {
		int concrete_value = 1;

		for (list<exprt>::const_iterator e_it = it->first.begin();
				e_it != it->first.end(); ++e_it) {
			exprt e = *e_it;

			if (e == loop_counter) {
				concrete_value *= num_unwindings;
			}
			else {
				map<exprt, int>::iterator v_it = values.find(e);

				if (v_it != values.end()) {
					concrete_value *= v_it->second;
				}
			}
		}

		typecast_exprt cast(it->second, expr_type);
		const mult_exprt term(from_integer(concrete_value, expr_type), cast);

		if (rhs.is_nil()) {
			rhs = term;
		}
		else {
			rhs = plus_exprt(rhs, term);
		}
	}

//	exprt overflow_expr;
//	overflow.overflow_expr(rhs, overflow_expr);

//	program.add_instruction(ASSUME)->guard = not_exprt(overflow_expr);

	rhs = typecast_exprt(rhs, target.type());

	const equal_exprt polynomial_holds(target, rhs);

	goto_programt::targett assumption = program.add_instruction(ASSUME);
	assumption->guard = polynomial_holds;
}

void acceleratort::fit_polynomial_sliced(goto_programt::instructionst &body,
		exprt &var,
		exprst &influence) {
	vector<list<exprt>> parameters;
	set<pair<list<exprt>, exprt> > coefficients;
	list<exprt> exprs;
	string dummy = "";
	cmdlinet c;
	ui_message_handlert mh(c, dummy);
	scratch_programt program(goto_model.symbol_table, mh);
//	goto_programt program;
//	exprt overflow_var = utils.fresh_symbol("polynomial::overflow",
//			bool_typet()).symbol_expr();
//	overflow_instrumentert overflow(program, overflow_var, symbol_table);
	auto loop_counter = nil_exprt();
	for (exprst::iterator it = influence.begin(); it != influence.end(); ++it) {
		if (it->id() == ID_index || it->id() == ID_dereference) {
			return;
		}

		exprs.clear();

		exprs.push_back(*it);
		parameters.push_back(exprs);

		exprs.push_back(loop_counter);
		parameters.push_back(exprs);
	}

	// N
	exprs.clear();
	exprs.push_back(loop_counter);
	parameters.push_back(exprs);

	// N^2
	exprs.push_back(loop_counter);
	parameters.push_back(exprs);

	// Constant
	exprs.clear();
	parameters.push_back(exprs);

	if (!(var.type().id() == ID_signedbv || var.type().id() == ID_unsignedbv)) {
		return;
	}

	size_t width = to_bitvector_type(var.type()).get_width();
	assert(width > 0);

	for (vector<list<exprt>>::iterator it = parameters.begin();
			it != parameters.end(); ++it) {
		auto coeff = create_symbol("polynomial::coeff", signedbv_typet(width));
		coefficients.insert(make_pair(*it, coeff.symbol_expr()));
	}
	map<exprt, int> values;

	for (exprst::iterator it = influence.begin(); it != influence.end(); ++it) {
		values[*it] = 0;
	}

	for (int n = 0; n <= 2; n++) {
		for (exprst::iterator it = influence.begin(); it != influence.end();
				++it) {
			values[*it] = 1;
			assert_for_values(program, values, coefficients, n, body, var);
			values[*it] = 0;
		}
	}

	assert_for_values(program, values, coefficients, 0, body, var);
	assert_for_values(program, values, coefficients, 2, body, var);

	for (exprst::iterator it = influence.begin(); it != influence.end(); ++it) {
		values[*it] = 2;
	}

	assert_for_values(program, values, coefficients, 2, body, var);

	goto_programt::targett assertion = program.add_instruction(ASSERT);
	assertion->guard = false_exprt();

	try {
		if (program.check_sat()) {
			cout << "CHeck sat worked!!!";
//			utils.extract_polynomial(program, coefficients, polynomial);
			return;
		}
	} catch (const string &s) {
		cout << "Error in fitting polynomial SAT check: " << s << '\n';
	} catch (const char *s) {
		cout << "Error in fitting polynomial SAT check: " << s << '\n';
	}

	return;
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
			for(auto a: src_syms) inf.insert(a);
			std::string s("i");
			z3_parse parser{};
			std::cout<<(parser.buildFormula(inf, s))<<std::endl;
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
