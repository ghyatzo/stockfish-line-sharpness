//
//  sharpness.hpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 20/10/2023.
//

#ifndef sharpness_hpp
#define sharpness_hpp

#include <stdio.h>
#include "stock_wrapper.hpp"


// We want to know what percentage of moves ends up in a overall worse position.
// we consider a blunder losing 300+ cp, a mistake losing 120-300 cp and inaccuracies 50-120 cp.
// depending on the ELO, you might consider changing these values.
// for now lets just put everything under one "bad move umbrella", all the blunders and all the mistakes.
// inaccuracies are considered neutral, the rest is good moves.
static constexpr double BLUNDER_THRESHOLD = 3; // blunders
static constexpr double MISTAKE_THRESHOLD = 1.1; // mistakes (sono scarso dio caro).
static constexpr double INACCURACY_THRESHOLD = 0.5; // inaccuracy

struct MoveDist {
    double good;
    double bad;
    double total;
};

namespace Sharpness {
    
    MoveDist MoveDistribution(const std::vector<double> &evals, double base_eval);
    MoveDist ComputePosition(Engine &engine, Position &pos);
    MoveDist ComputeMove(Stockfish::Move m, Engine &engine, Position& pos);
    
    double Ratio(const MoveDist &movedist);
    MoveDist ExpandRatio(const double ratio, const double blunders, const double total_moves);
    
    double Complexity(Engine& engine, Position& pos, int max_depth);
    std::vector<std::string>
    GenerateLine(size_t line_length, Position& pos, Engine& engine);
    
}
#endif /* sharpness_hpp */
