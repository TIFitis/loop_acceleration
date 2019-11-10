/*
 * accelerator.h
 *
 *  Created on: 07-Oct-2019
 *      Author: akash
 */
#include "loop_acceleration.h"
#include "z3_formula.h"

#ifndef ACCELERATOR_H_
#define ACCELERATOR_H_

#define ACC_N "__acc_n"
#define ACC_J "__acc_j"
#define ACC_P "__acc_p"

typedef std::set<exprt> exprst;

class loop_acc::acceleratort {
	goto_modelt &goto_model;
	std::map<goto_programt*, natural_loops_mutablet*> loops;

	void accelerate_all_functions();
	void accelerate_all_loops(goto_programt&);
	void accelerate_loop(const goto_programt::targett&,
			natural_loops_mutablet::natural_loopt&,
			goto_programt&);
	std::set<goto_programt*>& create_dup_loop(const goto_programt::targett&,
			natural_loops_mutablet::natural_loopt&,
			goto_programt&);
	void get_all_sources(exprt,
			goto_programt::instructionst&,
			exprst&,
			exprst&);
	void get_loops();
	void precondition(goto_programt &g_p,
			goto_programt::targett loc,
			goto_programt::targett sink,
			exprt loop_cond);
	bool check_pattern(code_assignt &, exprt);
	void add_overflow_checks(goto_programt &g_p);
	bool augment_path(goto_programt::targett &loop_header,
			goto_programt &functions,
			goto_programt &aux_path);
	bool syntactic_matching(goto_programt &g_p,
			goto_programt::instructionst &assign_insts,
			exprt loop_cond,
			goto_programt::targett sink);
	bool constraint_solver(goto_programt &g_p,
			goto_programt::instructionst &assign_insts,
			exprt &loop_cond,
			goto_programt::targett sink);
	void generate_paths(goto_programt &dup_body,
			goto_programt::targetst &branches,
			goto_programt::targetst::iterator cur_branch,
			std::map<goto_programt::targett, unsigned> path_explored, std::set<goto_programt*>& paths);
public:
	static symbolt create_symbol(std::string, const typet&, bool force = false);
	static exprst gather_syms(exprt, exprst&);
	static void swap_all(exprt &l_c, const exprt &n_e, const exprt &j_e);
	acceleratort(goto_modelt &goto_model) :
			goto_model(goto_model) {
	}
	bool accelerate();
	bool write_binary(std::string out_file) {
		cmdlinet c;
		std::string program = "";
		ui_message_handlert a(c, program);
		return write_goto_binary(out_file, goto_model, a);
	}
};

#endif /* ACCELERATOR_H_ */
