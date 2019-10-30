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
	std::string generate_arith(exprt expr);

public:
	z3_parse() = default;
	std::string buildFormula(std::set<exprt> &influence,
			const std::string &my_name,
			const goto_programt::instructionst assign_insts);

	exprt getAccFunc(exprt& n_e, const std::map<std::string, int>&);
};

#endif //LOOP_ACCELERATION_Z3_FORMULA_H
