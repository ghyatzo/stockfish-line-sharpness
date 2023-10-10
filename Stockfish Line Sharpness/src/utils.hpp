//
//  utils.hpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 09/10/2023.
//

#ifndef utils_hpp
#define utils_hpp

#include <stdio.h>
#include <string>
#include <iostream>
#include "mini_stock/position.h"
#include "mini_stock/movegen.h"

namespace Utils {
    using namespace Stockfish;
    
    char pt_to_char(PieceType pt);
    PieceType char_to_pt(char c);
    
    Square pt_to_sq(Position &pos, PieceType pt);
    Square coord_to_sq(std::string coord);
    
    std::string to_square(Square s);
    
    std::string to_alg(Position &pos, Move m);
    std::string to_long_alg(Move m);
    
    std::string alg_to_long(Position &pos, std::string alg);
    std::string long_to_alg(Position &pos, std::string str);
    
    Move alg_to_move(Position &pos, std::string m);
    Move long_alg_to_move(Position& pos, std::string m);
    
    void print_output(std::vector<std::string> & output, std::string prefix = "> ");
    
    double extract_eval(std::string eval_str);
    
    double centipawns(Color col, std::string &output);
    
    char pt_to_char(PieceType pt);
}

#endif /* utils_hpp */
