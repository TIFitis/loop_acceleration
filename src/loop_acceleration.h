/*
 * loop_acceleration.h
 *
 *  Created on: 07-Oct-2019
 *      Author: Akash
 */
#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <map>

#include <goto-programs/read_goto_binary.h>
#include <goto-programs/write_goto_binary.h>
#include <goto-programs/goto_model.h>
#include <analyses/natural_loops.h>
#include <langapi/language_util.h>
#include <langapi/mode.h>
#include <util/message.h>
#include <util/ui_message.h>
#include <util/cmdline.h>
#include <ansi-c/ansi_c_language.h>
#include <cpp/cpp_language.h>

#ifndef LOOP_ACCELERATION_H_
#define LOOP_ACCELERATION_H_

#endif /* LOOP_ACCELERATION_H_ */

namespace loop_acc {
class acceleratort;

void print_help();
void parse_input(int argc, char **argv, std::string&, std::string&);
}
