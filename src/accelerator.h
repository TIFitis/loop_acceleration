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

public:
	acceleratort(goto_modelt &goto_model) :
			goto_model(goto_model) {
	}
	bool accelerate();
	bool write_binary(std::string out_file) {
		messaget mh;
		return write_goto_binary(out_file, goto_model, mh.get_message_handler());
	}
};

#endif /* ACCELERATOR_H_ */
