/*
 * accelerator.h
 *
 *  Created on: 07-Oct-2019
 *      Author: akash
 */
#include "loop_acceleration.h"

#ifndef ACCELERATOR_H_
#define ACCELERATOR_H_

class loop_acc::acceleratort {
	goto_modelt &goto_model;
	std::map<goto_programt*, natural_loops_mutablet*> loops;

	void accelerate_all_functions();
	void accelerate_all_loops(goto_programt&);
	void accelerate_loop(goto_programt::targett&,
			natural_loops_mutablet::natural_loopt&,
			goto_programt&);
	void get_loops();

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
