/**
 * Assumption: Only 1 write per variable, and that write is at the end - no reads after that
 */

//
// Created by supreets51 on 26/10/19.
//

#include "z3_formula.h"
#include "accelerator.h"


std::string z3_parse::buildExprConst(std::set<exprt> &influence, const std::string& const_prefix){
    int counter = 0;

    auto it = influence.begin();
    std::string s("");
    s.append("(* ");
//    s.append(to_symbol_expr(*it).get_identifier().c_str());
    s.append(from_expr(*it));

    s.append(" ");
    s.append("a"+std::to_string(counter)+"_"+const_prefix+") ");
    counter+=1;

    it++;
    for(; it!=influence.end(); it++){
        std::string x ("(+ ");
        x.append(s);
        x.append(" (* ");
        x.append(from_expr(*it));
        x.append(" a"+std::to_string(counter)+"_"+const_prefix+") )");
        counter+=1;

        s = x;
    }

    return s;
}

std::string z3_parse::buildExprN(std::set <exprt> &influence, std::string &const_prefix) {
    std::string x ("(* (+ ");
    x.append(z3_parse::buildExprConst(influence, const_prefix));
    x.append(" a_N) N)");
    return x;
}

std::string z3_parse::buildCoeffs(std::set <exprt> &influence, std::string &const_prefix) {
    std::string x("");
    for(long unsigned int c =0; c<influence.size(); c++){
        x.append("(declare-const a"+std::to_string(c)+"_"+const_prefix+" Int)\n");
    }
    return x;
}

std::string z3_parse::buildInf(std::set <exprt> &influence) {
    std::string x("");
    for(auto a : influence){
        x.append("(declare-const "+from_expr(a)+" Int)\n");
    }
    x.append("(declare-const N Int)\n");
    x.append("(declare-const a_N Int)\n");
    x.append("(declare-const a_N2 Int)\n");

    return x;
}

std::string z3_parse::buildFunc(std::set <exprt> &influence) {
    std::string x("(+ (* a_N2 (* N N)) ( +");
    std::string c1("c");
    std::string c2("n");

    x.append(z3_parse::buildExprConst(influence, c1));
    x.append(" ");
    x.append(z3_parse::buildExprN(influence, c2));
    x.append(" ) )");

    return x;
}

// call this after buildInf
std::string z3_parse::buildAssert(std::set <exprt> &influence, const std::string &my_name) {
    std::string x("(declare-const "+ my_name+ "_new Int )\n");

    x.append("(assert (= N 1))\n");

    // Should change the values here using some path exec >..<
    for(auto a : influence){
        x.append("(assert (= 1 " + from_expr(a) + " ))\n");
    }

    x.append("(assert (= " + my_name + "_new " + buildFunc(influence) + " ))\n");

    return x;
}

std::string z3_parse::buildFormula(std::set <exprt> &influence, const std::string &my_name) {
    std::string x("");
    x.append(buildInf(influence));
    std::string c("c");
    x.append(buildCoeffs(influence,c ));
    c = "n";
    x.append(buildCoeffs(influence,c ));

    x.append(buildAssert(influence, my_name));

    x.append("(check-sat)\n(get-model)\n");

    return x;
}