//
// Created by supreets51 on 26/10/19.
//

#include "z3_formula.h"
#include "accelerator.h"


std::string z3_parse::buildExprConst(std::set<exprt> influence){
    int counter = 0;

    auto it = influence.begin();
    std::string s("");
    s.append("(* ");
    s.append(to_symbol_expr(*it).get_identifier().c_str());
    s.append(" ");
    s.append("a"+std::to_string(counter)+"_c) ");
    counter+=1;

    it++;
    for(; it!=influence.end(); it++){
        std::string x ("(+ ");
        x.append(s);
        x.append(" (* ");
        x.append(to_symbol_expr(*it).get_identifier().c_str());
        x.append("a"+std::to_string(counter)+"_c) ");

        s = x;
    }

    return s;
}

