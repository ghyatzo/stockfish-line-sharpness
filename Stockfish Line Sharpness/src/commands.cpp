//
//  commands.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 20/10/2023.
//
#include <string>
#include <iostream>

#include "stock_wrapper.hpp"
#include "utils.hpp"
#include "sharpness.hpp"
#include "commands.hpp"



//TODO: Make them more solid, incredibly janky as of now.
std::vector<double> LineSharpness(Engine &engine, const std::vector<Stockfish::Move> &moves, Position& pos)
{
    std::vector<double> sharpnesses {};
    sharpnesses.reserve(moves.size()+1);
    Position tmp {pos.fen()};
    
    auto ratio = Sharpness::Ratio(Sharpness::ComputePosition(engine, tmp));
    sharpnesses.emplace_back(ratio);
    
    for (int count {}; const auto mm : moves) {
        PROGRESS_BAR(count)
        tmp.DoMove(mm);
        ratio = Sharpness::Ratio(Sharpness::ComputePosition(engine, tmp));
        sharpnesses.emplace_back(ratio);
        
        ++count;
    }
    
    return sharpnesses;
}

double PositionSharpness(Engine &engine, Position &pos)
{
    auto movedist = Sharpness::ComputePosition(engine, pos);
    auto pos_complexity = Sharpness::Complexity(engine, pos, engine.Depth());
    // print the ratio
    
    double base_eval = engine.Eval(pos, Utils::lc0_cp_to_win);
    std::cout << "Eval: " << base_eval << " (depth: " << engine.Depth() << ")" << std::endl;
    std::cout << "In this position there are " << movedist.total << " possible moves.\n"
    << (pos.side_to_move() ? "Black" : "White") << " to move" << std::endl;
    std::cout << "\nBad moves: " << movedist.bad << " (loss >= " << MISTAKE_THRESHOLD << ")\n";
    std::cout << "Ok moves: " << movedist.good << " (loss < " << INACCURACY_THRESHOLD << ")\n";
    std::cout << "blunders: " << movedist.blunders << "\n";
    std::cout << "Sharpness ratio of: " << Sharpness::Ratio(movedist) << std::endl;
    std::cout << "Complexity score of: " << pos_complexity << std::endl;
    
    return Sharpness::Ratio(movedist);
}
