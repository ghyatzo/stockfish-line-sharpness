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
#include "position.hpp"

//#define PROGRESS_BAR(N, TOTAL) std::cout << std::string(TOTAL+2+2+std::floor(log10(N))+std::floor(log10(TOTAL)), ' ') << "\r" << \
//"[" << std::string(N, 'o') << std::string(TOTAL-N, '.') << "] " << N << "/" << TOTAL << "\r" << std::flush;
#define PROGRESS_BAR(count) { \
auto N = count; \
std::cout << std::string(N, ' ') << "\r" << \
"[" << std::string(N%6, '.') << std::string(5-(N%6), ' ') << "] " << "\r" << std::flush; \
}


static const auto MOVE_NONE_STR = "(none)";
static const auto MOVE_NULL_STR = "0000";
static const int NormalizeToPawnValue = 328;

namespace Utils {

    char pt_to_char(Stockfish::PieceType pt);
    Stockfish::PieceType char_to_pt(char c);
    
    Stockfish::Square pt_to_sq(Position &pos, Stockfish::PieceType pt);
    Stockfish::Square coord_to_sq(std::string coord);
    
    std::string to_coord(Stockfish::Square s);
    
    std::string to_alg(Position &pos, Stockfish::Move m);
    std::string to_long_alg(Stockfish::Move m);
    
    std::string alg_to_long(Position &pos, std::string alg);
    std::string long_to_alg(Position &pos, std::string str);
    
    Stockfish::Move alg_to_move(Position &pos, std::string m);
    Stockfish::Move long_alg_to_move(Position& pos, std::string m);
    
    std::vector<Stockfish::Move> translate_moves(::Position& pos,
                                                 std::vector<std::string> &moves,
                                                 bool short_algebraic_notation = true);
    
    void print_output(const std::vector<std::string> & output, std::string prefix = "> ");
    std::string parse_best_move(const std::vector<std::string> & output);
    std::string parse_score(const std::vector<std::string> & output);
    std::tuple<int, int, int> parse_wdl(const std::vector<std::string> & output);
    
    inline Stockfish::Value cp_to_value(int cp) { return Stockfish::Value(cp * NormalizeToPawnValue / 100); }
    inline int to_cp(Stockfish::Value v) { return 100 * v / NormalizeToPawnValue; }
    double centipawns(Stockfish::Color col, const std::vector<std::string> & output);
    double format_cp(Stockfish::Color col, double cp);
    double lichess_cp_to_win(double cp);
    double lc0_cp_to_win(double cp);
    
    void sort_evals_perm(std::vector<int> &perm, const std::vector<double> &evals, Stockfish::Color col);
    std::vector<int> sort_evals_perm(const std::vector<double> &evals, Stockfish::Color col);

}

#endif /* utils_hpp */
