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

#define PROGRESS_BAR(N, TOTAL) std::cout << std::string(TOTAL+2+2+std::floor(log10(N))+std::floor(log10(TOTAL)), ' ') << "\r" << \
"[" << std::string(N, 'o') << std::string(TOTAL-N, '.') << "] " << N << "/" << TOTAL << "\r" << std::flush;


static const auto MOVE_NONE_STR = "(none)";
static const auto MOVE_NULL_STR = "0000";
static const int NormalizeToPawnValue = 328;

namespace Utils {
    using namespace Stockfish;

    char pt_to_char(PieceType pt);
    PieceType char_to_pt(char c);
    
    Square pt_to_sq(Position &pos, PieceType pt);
    Square coord_to_sq(std::string coord);
    
    std::string to_coord(Square s);
    
    std::string to_alg(Position &pos, Move m);
    std::string to_long_alg(Move m);
    
    std::string alg_to_long(Position &pos, std::string alg);
    std::string long_to_alg(Position &pos, std::string str);
    
    Move alg_to_move(Position &pos, std::string m);
    Move long_alg_to_move(Position& pos, std::string m);
    
    void print_output(const std::vector<std::string> & output, std::string prefix = "> ");
    std::string parse_best_move(const std::vector<std::string> & output);
    std::string parse_score(const std::string & info_line);
    std::tuple<int, int, int> parse_wdl(const std::string & info_line);
    
    Value cp_to_value(int cp);
    int to_cp(Value v);
    double centipawns(Color col, const std::string &output);
    
    char pt_to_char(PieceType pt);
    
    void sort_evals_perm(std::vector<int> &perm, const std::vector<double> &evals);
    std::vector<int> sort_evals_perm(const std::vector<double> &evals);

}

#endif /* utils_hpp */
