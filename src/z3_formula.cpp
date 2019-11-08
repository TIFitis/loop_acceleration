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
		s.append(from_expr(expr));
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
	for (unsigned i = 1; i <= 2 * (k + 1); i++) {
		formula.append("(declare-const alpha_" + std::to_string(i) + " Int)\n");
//		formula.append("(assert (>= alpha_" + std::to_string(i) + " 0))\n");
	}
}

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
	std::map<exprt, exprt> latest_sym;
	for (auto inst : assign_insts) {
		latest_sym[inst.code.op0()] = inst.code.op0();
	}
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
		lhs = "__acc_" + eq_n_str + std::to_string(ssa_count) + lhs;
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
			loop_acc::acceleratort::swap_all(temp,
					latest_sym[ins_c.lhs()],
					new_copy);
			it2->code.op1().swap(temp);
		}
		latest_sym[ins_c.lhs()] = new_copy;
	}
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
				temp2 = "(+ " + temp2 + " (* (* alpha_"
						+ std::to_string(k + j + 1) + " "
						+ std::to_string(input_set[i][j + 1]) + ") " + n_str
						+ "))";
			}
			else
				temp2 = "(* (* alpha_" + std::to_string(k + j + 1) + " "
						+ std::to_string(input_set[i][j + 1]) + ") " + n_str
						+ ")";
		}
		temp1 = "(+ " + temp1 + " " + temp2 + ")";
		temp1 = "(+ " + temp1 + " (* alpha_" + std::to_string(2 * k + 1) + " "
				+ n_str + "))";

		temp1 = "(+ " + temp1 + " (* alpha_" + std::to_string(2 * (k + 1)) + " "
				+ "(* " + n_str + " " + n_str + ")))";
		std::string rhs;
		if (n_val == 0) {
			unsigned loc = 1u;
			for (auto a : loop_vars) {
				if (a == x) break;
				loc++;
			}
			rhs = std::to_string(input_set[i][loc]);
		}
		else
			rhs = add_symex(assign_insts, tgt_inst, loop_vars, i);
		temp1 = "(= " + rhs + " " + temp1 + ")";
		temp1 = "(assert " + temp1 + ")\n";
		formula.append(temp1);
	}
}

bool z3_parse::z3_fire() {
	FILE *fp = fopen("z3_input.smt", "w");
	assert(fp != nullptr && "couldnt create input file for z3");
	formula.append("(check-sat)\n");
	for (auto i = 1u; i <= input_set[0].size() * 2; i++) {
		formula.append("(get-value (alpha_" + std::to_string(i) + "))\n");
	}
#if DBGLEVEL >= 5
			std::cout << "\n========Formula==========\n"
			<< formula
			<< "========Formula==========\n"
			<< std::endl;
#endif
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
	if (raw_input.find("unsat") != raw_input.npos) return values;
	if (raw_input.find("error") != raw_input.npos) return values;

	while (1) {
		auto loc = raw_input.find("(");
		if (loc == raw_input.npos) break;
		raw_input = raw_input.substr(loc + 2, raw_input.npos);
		loc = raw_input.find(" ");
		auto name = raw_input.substr(0, loc);
		raw_input = raw_input.substr(loc + 1, raw_input.npos);
		loc = raw_input.find(")");
		auto val_s = raw_input.substr(0, loc);
		bool minus_flag = false;
		if (val_s[0] == '(') {
			val_s = val_s.substr(3, val_s.npos);
			minus_flag = true;
		}
		std::stringstream val_ss(val_s);
		int x = 0;
		val_ss >> x;
		if (minus_flag) x = -x;
		values[name] = x;
	}

#if DBGLEVEL >= 5
	for (auto a : values)
		std::cout << a.first << "::" << a.second << std::endl;
#endif
	return values;
}

exprt z3_parse::getAccFunc(const std::map<std::string, int> &coeff_vals,
		exprst loop_vars,
		exprt n_e) {
	std::vector<exprt> r_map;
	for (auto e : loop_vars) {
		r_map.push_back(e);
	}
	auto x = coeff_vals.at("alpha_" + std::to_string(1));
	exprt expr = mult_exprt(from_integer(x, signedbv_typet(32)), r_map[0]);
	auto k = input_set[0].size() - 1;
	for (auto i = 2u; i <= k; i++) {
		auto x = coeff_vals.at("alpha_" + std::to_string(i));
		expr = plus_exprt(expr,
				mult_exprt(from_integer(x, signedbv_typet(32)), r_map[i - 1]));
	}
	for (auto i = 1u; i <= k; i++) {
		auto x = coeff_vals.at("alpha_" + std::to_string(i + k));
		expr = plus_exprt(expr,
				mult_exprt(mult_exprt(from_integer(x, signedbv_typet(32)),
						r_map[i - 1]),
						n_e));
	}
	x = coeff_vals.at("alpha_" + std::to_string(2 * k + 1));
	expr = plus_exprt(expr,
			mult_exprt(from_integer(x, signedbv_typet(32)), n_e));
	x = coeff_vals.at("alpha_" + std::to_string(2 * k + 2));
	expr = plus_exprt(expr,
			mult_exprt(mult_exprt(from_integer(x, signedbv_typet(32)), n_e),
					n_e));
	return expr;
}
