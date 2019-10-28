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

typedef std::unordered_set<exprt, irep_hash> exprst;

class loop_acc::acceleratort {
	goto_modelt &goto_model;
	std::map<goto_programt*, natural_loops_mutablet*> loops;

	void accelerate_all_functions();
	void accelerate_all_loops(goto_programt&);
	void accelerate_loop(goto_programt::targett&,
			natural_loops_mutablet::natural_loopt&,
			goto_programt&);
	goto_programt& create_dup_loop(goto_programt::targett&,
			natural_loops_mutablet::natural_loopt&,
			goto_programt&);
	void get_all_sources(exprt,
			goto_programt::instructionst&,
			exprst&,
			goto_programt::instructionst&);
	exprst gather_syms(exprt);
	void get_loops();
	symbolt create_symbol(std::string, const typet&);
	std::map<std::string, int> get_z3_model(std::string);
	bool z3_fire(const std::string&);
	exprt precondition(goto_programt &);
	bool check_pattern(code_assignt &, exprt);
	bool augment_path(goto_programt::targett &loop_header,
			goto_programt &functions,
			goto_programt &aux_path);
public:
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
