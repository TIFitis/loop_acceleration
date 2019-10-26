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

namespace z3_parse{
    // change this type to set of exprs
    // need this set for some kind of order

    std::string buildExprConst(std::set<exprt> influence);

}


#endif //LOOP_ACCELERATION_Z3_FORMULA_H
