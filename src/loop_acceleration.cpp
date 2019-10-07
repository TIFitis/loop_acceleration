//============================================================================
// Name        : loop_acceleration.cpp
// Author      : Akash
// Version     :
// Copyright   : 
// Description : Loop-Accelerator
//============================================================================

#include "loop_acceleration.h"
#include "accelerator.h"

using namespace loop_acc;
using namespace std;

int main(int argc, char **argv) {
	string in_file, out_file;
	parse_input(argc, argv, in_file, out_file);
	messaget mh;
	auto gb = read_goto_binary(in_file, mh.get_message_handler());
	acceleratort acc(gb.value());
	if (acc.accelerate()) cout << "Accelereation Failed!!!" << endl;
	acc.write_binary(out_file);
	return 0;
}

