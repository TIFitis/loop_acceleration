/*
 * loop_acceleration.h
 *
 *  Created on: 07-Oct-2019
 *      Author: Akash
 */
#include <bits/stdc++.h>
#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include <analyses/natural_loops.h>
#include <ansi-c/ansi_c_language.h>
#include <cpp/cpp_language.h>
#include <goto-programs/goto_model.h>
#include <goto-programs/goto_program.h>
#include <goto-programs/goto_inline.h>
#include <goto-programs/remove_function_pointers.h>
#include <goto-programs/remove_virtual_functions.h>
#include <goto-programs/remove_asm.h>
#include <goto-programs/remove_calls_no_body.h>
#include <goto-programs/read_goto_binary.h>
#include <goto-programs/remove_skip.h>
#include <goto-programs/write_goto_binary.h>
#include <langapi/language_util.h>
#include <langapi/mode.h>
#include <util/arith_tools.h>
#include <util/cmdline.h>
#include <util/c_types.h>
#include <util/expr.h>
#include <util/message.h>
#include <util/ui_message.h>
#include <util/replace_expr.h>
#include <util/simplify_expr.h>
#include <util/std_code.h>

#ifndef LOOP_ACCELERATION_H_
#define LOOP_ACCELERATION_H_

#endif /* LOOP_ACCELERATION_H_ */

namespace loop_acc {
class acceleratort;

void print_help();
void parse_input(int argc, char **argv, std::string&, std::string&);
}
