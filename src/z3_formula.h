//
// Created by supreets51 on 26/10/19.
//

#ifndef LOOP_ACCELERATION_Z3_FORMULA_H
#define LOOP_ACCELERATION_Z3_FORMULA_H
#include <string>
#include <vector>
#include <set>
#include <map>
#include "accelerator.h"

class z3_parse {
	// change this type to set of exprs
	// need this set for some kind of order
private:
	std::string formula;
	std::vector<std::vector<int>> input_set;
	void build_input_set(unsigned k);
	void add_alpha_decl(unsigned k);
	void add_assert(unsigned k);
	std::string add_symex(const goto_programt::instructionst &assign_insts,
			const goto_programt::instructiont &inst,
			const exprst &loop_vars,
			unsigned eqn);

	std::set<exprt> influ_set;

	std::string buildExprConst(std::set<exprt> &influence,
			const std::string &const_prefix);
	std::string buildExprN(std::set<exprt> &influence,
			std::string &const_prefix);
	std::string buildCoeffs(std::set<exprt> &influence,
			std::string &const_prefix);
	std::string buildInf(std::set<exprt> &influence);

	std::string buildFunc(std::set<exprt> &influence);
	std::string buildAssert(std::set<exprt> &influence,
			const std::string &my_name);
	std::string buildInsts(const goto_programt::instructionst &assign_insts);
	std::string generate_arith(exprt expr, std::string s1 = "", std::string s2 =
			"");

public:
	z3_parse() {
		std::vector<int> a { 0, 1 };
		input_set.push_back(a);
		std::vector<int> b { 1, 0 };
		input_set.push_back(b);
		std::vector<int> c { 1, 1 };
		input_set.push_back(c);
		std::vector<int> d { 2, 1 };
		input_set.push_back(d);
	}
	void new_build(exprst &influence,
			exprt x,
			const goto_programt::instructionst &assign_insts,
			goto_programt::instructiont &inst);

	std::string buildFormula(std::set<exprt> &influence,
			const std::string &my_name,
			const goto_programt::instructionst assign_insts);

	exprt getAccFunc(exprt &n_e, const std::map<std::string, int>&);
	bool z3_fire(const std::string &z3_formula);
	bool z3_fire();
	std::map<std::string, int> get_z3_model(std::string);
};

#endif //LOOP_ACCELERATION_Z3_FORMULA_H
