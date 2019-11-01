/**
 * Assumption: Only 1 write per variable, and that write is at the end - no reads after that
 * a<num>_n / a<num>_c, a_N, a_N2
 */

//
// Created by supreets51 on 26/10/19.
//
#include "z3_formula.h"
#include "accelerator.h"
#include "../cbmc/src/util/std_expr.h"

std::string z3_parse::buildExprConst(std::set<exprt> &influence,
		const std::string &const_prefix) {
	int counter = 0;

	auto it = influence.begin();
	std::string s("");
	s.append("(* ");
//    s.append(to_symbol_expr(*it).get_identifier().c_str());
	s.append(from_expr(*it));

	s.append(" ");
	s.append("a" + std::to_string(counter) + "_" + const_prefix + ") ");
	counter += 1;

	it++;
	for (; it != influence.end(); it++) {
		std::string x("(+ ");
		x.append(s);
		x.append(" (* ");
		x.append(from_expr(*it));
		x.append(" a" + std::to_string(counter) + "_" + const_prefix + ") )");
		counter += 1;

		s = x;
	}

	return s;
}

std::string z3_parse::buildExprN(std::set<exprt> &influence,
		std::string &const_prefix) {
	std::string x("(* (+ ");
	x.append(z3_parse::buildExprConst(influence, const_prefix));
	x.append(" a_N) N)");
	return x;
}

std::string z3_parse::buildCoeffs(std::set<exprt> &influence,
		std::string &const_prefix) {
	std::string x("");
	for (long unsigned int c = 0; c < influence.size(); c++) {
		x.append("(declare-const a" + std::to_string(c) + "_" + const_prefix
				+ " Int)\n");
	}
	return x;
}

std::string z3_parse::buildInf(std::set<exprt> &influence) {
	std::string x("");
	for (auto a : influence) {
		x.append("(declare-const " + from_expr(a) + " Int)\n");
	}
	x.append("(declare-const N Int)\n");
	x.append("(declare-const a_N Int)\n");
	x.append("(declare-const a_N2 Int)\n");

	return x;
}

std::string z3_parse::buildFunc(std::set<exprt> &influence) {
	std::string x("(+ (* a_N2 (* N N)) ( +");
	std::string c1("c");
	std::string c2("n");

	x.append(z3_parse::buildExprConst(influence, c1));
	x.append(" ");
	x.append(z3_parse::buildExprN(influence, c2));
	x.append(" ) )");

	return x;
}

// call this after buildInf
std::string z3_parse::buildAssert(std::set<exprt> &influence,
		const std::string &my_name) {
	std::string x("(declare-const " + my_name + "_new Int )\n");

	x.append("(assert (= N 1))\n");
//	x.append("(assert (>= N 0))\n");
//	x.append("(assert (<= N 2))\n");

// Should change the values here using some path exec >..<
// Should add something like
	for (auto a : influence) {
//		x.append("(assert (< 0 " + from_expr(a) + " ))\n");
//        x.append("(assert (> 10 " + from_expr(a) + " ))\n");
		x.append("(assert (= 1 " + from_expr(a) + " ))\n");

	}

	x.append("(assert (= " + my_name + "_new " + buildFunc(influence)
			+ " ))\n");

	return x;
}

std::string z3_parse::generate_arith(exprt expr,
		std::string s1,
		std::string s2) {
	std::string s = "";
	std::string exp_s = from_expr(expr);
	if (can_cast_expr<symbol_exprt>(expr)) {
		if (!from_expr(expr).compare(s1)) {
			s.append(s2);
		}
		else
			s.append(from_expr(expr));
	}
	else if (can_cast_expr<constant_exprt>(expr)) {
		auto const_expr = to_constant_expr(expr);
		s.append(const_expr.get_value().c_str());
	}
	else if (can_cast_expr<plus_exprt>(expr)) {
		auto plus_expr = to_plus_expr(expr);
		s.append("(+ " + generate_arith(plus_expr.op0()) + " "
				+ generate_arith(plus_expr.op1(), s1, s2) + ")");

	}
	else if (can_cast_expr<minus_exprt>(expr)) {
		auto minus_expr = to_minus_expr(expr);
		s.append("(- " + generate_arith(minus_expr.op0(), s1, s2) + " "
				+ generate_arith(minus_expr.op1(), s1, s2) + ")");

	}
	else if (can_cast_expr<mult_exprt>(expr)) {
		auto mult_expr = to_mult_expr(expr);
		s.append("(* " + generate_arith(mult_expr.op0(), s1, s2) + " "
				+ generate_arith(mult_expr.op1(), s1, s2) + ")");

	}
	else if (can_cast_expr<div_exprt>(expr)) {
		auto mult_expr = to_div_expr(expr);
		s.append("(/ " + generate_arith(mult_expr.op0(), s1, s2) + " "
				+ generate_arith(mult_expr.op1(), s1, s2) + ")");

	}
	else {
		std::cerr << from_expr(expr)
				<< " has type "
				<< expr.type().id().c_str()
				<< std::endl;
		assert(false && "Only arith instructions are handled!");
	}
	return s;
}

std::string z3_parse::buildInsts(const goto_programt::instructionst &assign_insts) {
	std::string x("");
	for (auto &inst : assign_insts) {
		auto &inst_code = to_code_assign(inst.code);
		x.append("(assert(= " + from_expr(inst_code.lhs()) + "_new" + " "
				+ generate_arith(inst_code.rhs()) + "))\n");
	}
	return x;
}

std::string z3_parse::buildFormula(std::set<exprt> &influence,
		const std::string &my_name,
		const goto_programt::instructionst assign_insts) {

	this->influ_set = influence;

	std::string x("");
	x.append(buildInf(influence));
	std::string c("c");
	x.append(buildCoeffs(influence, c));
	c = "n";
	x.append(buildCoeffs(influence, c));

	x.append(buildAssert(influence, my_name));
	x.append(buildInsts(assign_insts));
	x.append("(check-sat)\n(get-model)\n");

	return x;
}

void z3_parse::build_input_set(unsigned k) {
	while (input_set[0].size() < (k + 1)) {
		for (auto &a : input_set) {
			if (a[0] == 2) {
				a.push_back(1);
			}
			else
				a.push_back(0);
		}
		std::vector<int> n1(input_set[0].size(), 0);
		n1.back() = 1;
		input_set.push_back(n1);
		n1[0] = 1;
		input_set.push_back(n1);
	}
}

void z3_parse::add_alpha_decl(unsigned k) {
	for (unsigned i = 1; i <= 2 * (k + 1); i++)
		formula.append("(declare-const alpha_" + std::to_string(i) + " Int)\n");
}

//void z3_parse::add_assert(exprt tgt, exprst src_syms, unsigned k) {
//	for (unsigned i = 1; i <= k; i++)
//		formula.append("(declare-const alpha_" + std::to_string(i) + " Int)\n");
//}

std::string z3_parse::add_symex(const goto_programt::instructionst &assign_insts_o,
		const goto_programt::instructiont &tgt_inst,
		const exprst &loop_vars,
		unsigned eq_n) {
	std::string ret_str;
	static unsigned ssa_count;
	std::map<std::string, unsigned> k_vars;
	auto eq_n_str = std::to_string(eq_n) + "_";
	auto assign_insts = assign_insts_o;
	if (input_set[eq_n][0] == 2) {
		for (auto a : assign_insts_o)
			assign_insts.push_back(a);
	}
//	std::cout << "before ssa ====================\n\n";
//	for (auto a : assign_insts)
//		std::cout << from_expr(a.code) << std::endl;
	unsigned i = 1;
	for (auto e : loop_vars) {
		k_vars[from_expr(e)] = i;
		i++;
	}
	exprst my_syms;
	for (auto it = assign_insts.begin(), it_e = assign_insts.end(); it != it_e;
			) {
		auto ins_c = to_code_assign(it->code);
		auto rhs_e = ins_c.rhs();
		exprst rhs_syms;
		loop_acc::acceleratort::gather_syms(rhs_e, rhs_syms);
		for (auto &a : rhs_syms) {
			if (my_syms.find(a) != my_syms.end()) {
				rhs_syms.erase(a);
			}
		}
		for (auto sym : rhs_syms) {
			auto ip_val = from_integer(input_set[eq_n][k_vars[from_expr(sym)]],
					sym.type());
			loop_acc::acceleratort::swap_all(rhs_e, sym, ip_val);
		}
		auto lhs = from_expr(ins_c.lhs());
		lhs = "acc_" + eq_n_str + std::to_string(ssa_count) + lhs;
		ssa_count++;
		auto n_lhs_sym = loop_acc::acceleratort::create_symbol(lhs,
				ins_c.lhs().type(),
				true).symbol_expr();
		my_syms.insert(n_lhs_sym);
		auto new_copy = n_lhs_sym;
		it->code.op0().swap(n_lhs_sym);
		it->code.op1().swap(rhs_e);
		if (it->location_number == tgt_inst.location_number) {
			ret_str = lhs;
		}
		it++;
		for (auto it2 = it, it_e2 = assign_insts.end(); it2 != it_e2; it2++) {
			auto temp = it2->code.op1();
			loop_acc::acceleratort::swap_all(temp, ins_c.lhs(), new_copy);
			it2->code.op1().swap(temp);
		}
	}
//	std::cout << "\n\nPure Bliss====================\n";
	for (auto a : assign_insts) {
		std::string t1 = "(declare-const " + from_expr(a.code.op0())
				+ " Int)\n";
		formula.append(t1);
	}
	for (auto a : assign_insts) {
		std::string t1 = "(assert (= " + from_expr(a.code.op0()) + " "
				+ generate_arith(a.code.op1()) + "))\n";
		formula.append(t1);
	}
	return ret_str;
}

void z3_parse::new_build(exprst &loop_vars,
		exprt x,
		const goto_programt::instructionst &assign_insts,
		goto_programt::instructiont &tgt_inst) {
	auto inst = to_code_assign(tgt_inst.code);
	unsigned k = loop_vars.size();
	build_input_set(k);
	add_alpha_decl(k);
	std::cout << "=================================" << std::endl;
	std::map<std::string, unsigned> ssa_vals;
	for (unsigned i = 0; i < 2 * (k + 1); i++) {
		auto n_val = input_set[i][0];
		std::string n_str = std::to_string(input_set[i][0]);
		std::string temp1;
		for (unsigned j = 0; j < k; j++) {
			if (j > 0) {
				temp1 = "(+ " + temp1 + " (* alpha_" + std::to_string(j + 1)
						+ " " + std::to_string(input_set[i][j + 1]) + "))";
			}
			else
				temp1 = "(* alpha_" + std::to_string(j + 1) + " "
						+ std::to_string(input_set[i][j + 1]) + ")";
		}
		std::string temp2;
		for (unsigned j = 0; j < k; j++) {
			if (j > 0) {
				temp2 = "(+ " + temp2 + " (* alpha_" + std::to_string(k + j + 1)
						+ " " + std::to_string(input_set[i][j + 1]) + ") "
						+ ")";
			}
			else
				temp2 = "(* alpha_" + std::to_string(k + j + 1) + " "
						+ std::to_string(input_set[i][j + 1]) + ")";
		}
		temp2 = "(* " + temp2 + " " + n_str + ")";
		temp1 = "(+ " + temp1 + " " + temp2 + ")";
		temp1 = "(+ " + temp1 + " (* alpha_" + std::to_string(2 * k + 1) + " "
				+ n_str + "))";

		temp1 = "(+ " + temp1 + " (* alpha_" + std::to_string(2 * (k + 1)) + " "
				+ "(* " + n_str + " " + n_str + ")))";
		std::string rhs;
		if (n_val == 0) {
			rhs = std::to_string(input_set[i][1]);
		}
		else
			rhs = add_symex(assign_insts, tgt_inst, loop_vars, i);
//		else if (n_val == 1) {
//			for (auto ins : assign_insts) {
//				std::string s = generate_arith(inst.rhs(),
//						from_expr(x),
//						std::to_string(input_set[i][1]));
//			}
//			formula.append("(assert (= acc_" + std::to_string(i + 1) + " " + rhs
//					+ "))\n");
////			rhs = generate_arith(inst.rhs(),
////					from_expr(x),
////					std::to_string(input_set[i][1]));
//		}
//		else if (n_val == 2) {
//			rhs = generate_arith(inst.rhs(),
//					from_expr(x),
//					std::to_string(input_set[i][1]));
//			formula.append("(assert (= acc_" + std::to_string(i + 1) + " " + rhs
//					+ "))\n");
//			rhs = generate_arith(inst.rhs(),
//					from_expr(x),
//					"acc_" + std::to_string(i + 1));
//		}
		temp1 = "(= " + rhs + " " + temp1 + ")";
		temp1 = "(assert " + temp1 + ")\n";
		formula.append(temp1);
	}
	std::cout << formula << std::endl;
}

bool z3_parse::z3_fire(const std::string &z3_formula) {
	FILE *fp = fopen("z3_input.smt", "w");
	assert(fp != nullptr && "couldnt create input file for z3");
	fputs(z3_formula.c_str(), fp);
	fclose(fp);
	std::string z3_command = "z3 -smt2 z3_input.smt > z3_results.dat";
	system(z3_command.c_str());
	return false;
}

bool z3_parse::z3_fire() {
	FILE *fp = fopen("z3_input.smt", "w");
	assert(fp != nullptr && "couldnt create input file for z3");
	formula.append("(check-sat)\n(get-model)\n");
	fputs(formula.c_str(), fp);
	fclose(fp);
	std::string z3_command = "z3 -smt2 z3_input.smt > z3_results.dat";
	system(z3_command.c_str());
	return false;
}

std::map<std::string, int> z3_parse::get_z3_model(std::string filename) {
	FILE *fp;
	fp = fopen(filename.c_str(), "r");
	assert(fp != nullptr && "Could not open z3_results.dat file");
	char s[4096];
	std::string raw_input = "";
	while (fgets(s, 4096, fp))
		raw_input += s;
	fclose(fp);
	std::map<std::string, int> values;
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
		std::stringstream val_ss(val_s);
		int x = 0;
		val_ss >> x;
		values[name] = x;
	}
	for (auto a : values)
		std::cout << a.first << "::" << a.second << std::endl;
	return values;
}

exprt z3_parse::getAccFunc(exprt &n_e,
		const std::map<std::string, int> &coeff_vals) {
//auto l_e = to_symbol_expr(inst_c.lhs());

	int ctr = 0;
	exprt add_expr_c;
// Without N terms
	for (auto e : this->influ_set) {
		auto x = coeff_vals.find("a" + std::to_string(ctr) + "_c");
		if (x == coeff_vals.end()) {
			assert(false && "getAccFunc: constant not found in coeff_vals!");
			return nil_exprt();
		}
		auto expr1 = mult_exprt(e, from_integer(x->second, e.type()));
		if (ctr == 0)
			add_expr_c = expr1;
		else
			add_expr_c = plus_exprt(add_expr_c, expr1);

		ctr++;
	}

//std::cout<< "Made expr till now: " << from_expr(add_expr_c) <<std::endl;

// with N terms
	exprt add_expr_n;
	ctr = 0;
	for (auto e : this->influ_set) {
		auto x = coeff_vals.find("a" + std::to_string(ctr) + "_n");
		if (x == coeff_vals.end()) {
			assert(false && "getAccFunc: constant not found in coeff_vals!");
			return nil_exprt();
		}
		auto expr1 = mult_exprt(e, from_integer(x->second, e.type()));
		if (ctr == 0)
			add_expr_n = expr1;
		else
			add_expr_n = plus_exprt(add_expr_c, expr1);

		ctr++;
	}

	auto x = coeff_vals.find("a_N");
	if (x == coeff_vals.end()) {
		assert(false && "getAccFunc: constant a_N not found in coeff_vals!");
		return nil_exprt();
	}
	add_expr_n = plus_exprt(add_expr_n,
			from_integer(x->second, add_expr_c.type()));

	add_expr_n = mult_exprt(add_expr_n, n_e);

	add_expr_c = plus_exprt(add_expr_c, add_expr_n);

	add_expr_c = plus_exprt(add_expr_c,
			mult_exprt(mult_exprt(n_e, n_e),
					from_integer(coeff_vals.find("a_N2")->second,
							add_expr_c.type())));

	return add_expr_c;
}
