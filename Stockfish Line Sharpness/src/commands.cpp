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
std::vector<double> whole_line_sharpness(Stock &stock, const std::vector<Move> &moves, int depth)
{
    std::vector<double> sharpnesses {};
    sharpnesses.reserve(moves.size()+1);
    
    auto ratio = Sharpness::Ratio(Sharpness::Position(stock, depth));
    sharpnesses.emplace_back(ratio);
    
    for (int count {}; const auto mm : moves) {
        PROGRESS_BAR(count, moves.size())
        stock.pos.do_move(mm);
        ratio = Sharpness::Ratio(Sharpness::Position(stock, depth));
        sharpnesses.emplace_back(ratio);
        
        ++count;
    }
    
    return sharpnesses;
}

double end_position_sharpness(Stock &stock, const std::vector<Move> &starting_moves, int depth)
{
    stock.AdvancePosition(starting_moves);
    std::cout << stock.pos << std::endl;
    
    auto movedist = Sharpness::Position(stock, depth);
    auto pos_complexity = Sharpness::Complexity(stock, depth);
    // print the ratio
    
    double base_eval = stock.EvalPosition(depth, -1);
    std::cout << "Eval: " << base_eval << " (depth: " << depth << ")" << std::endl;
    std::cout << "In This position there are " << movedist.total << " possible moves.\n"
    << (stock.pos.side_to_move() ? "Black" : "White") << " to move" << std::endl;
    std::cout << "Bad moves: " << movedist.bad << " (loss >= " << MISTAKE_THRESHOLD << ")\n";
    std::cout << "Ok moves: " << movedist.good << " (loss < " << INACCURACY_THRESHOLD << ")\n";
    std::cout << "blunders: " << movedist.blunders << "\n";
    std::cout << "Sharpness ratio of: " << Sharpness::Ratio(movedist) << std::endl;
    std::cout << "Complexity score of: " << pos_complexity << std::endl;
    
    return Sharpness::Ratio(movedist);
}
