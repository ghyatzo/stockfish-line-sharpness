//
//  sharpness.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 20/10/2023.
//

#include <string>
#include <vector>
#include <iostream>
#include <numeric>

#include "sharpness.hpp"
#include "utils.hpp"
#include "stock_wrapper.hpp"
#include "mini_stock/movegen.h"
#include "mini_stock/position.h"

namespace Sharpness {
    
    MoveDist
    MoveDistribution(const std::vector<double> &evals, double base_eval)
    {
        double ok_moves {};
        double bad_moves {};
        double blunders {};
        for (auto e : evals) {
            double delta = std::abs(base_eval - e);
            if (delta >= BLUNDER_THRESHOLD) { blunders++; continue; }
            
            if (delta <= INACCURACY_THRESHOLD) { ok_moves++; continue; }
            if (delta > INACCURACY_THRESHOLD && delta <= MISTAKE_THRESHOLD) {
                ok_moves += 0.5; bad_moves += 0.5; continue; }
            if (delta > MISTAKE_THRESHOLD) { bad_moves++; }
        }
        
        return {ok_moves, bad_moves, blunders, static_cast<double>(evals.size())};
    }
    
    MoveDist
    Position(Stock &stock, int depth)
    {
        double base_eval = stock.EvalPosition(depth, -1);
        auto evals = stock.EvalMoves(stock.GetMoves(), depth, -1);
        
        return MoveDistribution(evals, base_eval);
    }
    
    // TODO: these still needs work.
    MoveDist Move(Stockfish::Move m, Stock &stock, int depth)
    {
        // how sharp is this move: playing this move generates a position, how sharp is that position?
        // how sharp is the resulting position for the adversary?
        stock.pos.do_move(m);
        auto base_eval = stock.EvalPosition(depth);
        auto evals = stock.EvalMoves(stock.GetMoves(), depth);
        return MoveDistribution(evals, base_eval);
    }
    
    double Ratio(const MoveDist &movedist)
    {
        // old iteration
//        return (movedist.blunders + movedist.bad) / movedist.total_moves;
        // last iteration, we want the ratio of (bad)/(ok+bad) moves, ignoring blunders
        return movedist.bad / (movedist.bad + movedist.good);
        
    }
    
    MoveDist ExpandRatio(double ratio, double blunders, double total_moves)
    {
        // ratio = bad_moves/(good_moves + bad_moves) and num_moves - blunders = good_moves + bad_moves.
        auto bad_moves = ratio*(total_moves-blunders);
        auto good_moves = total_moves-blunders-bad_moves;
        return {good_moves, bad_moves, static_cast<double>(blunders), static_cast<double>(total_moves)};
    }
    
    double Complexity(Stock& stock, int max_depth)
    {
        // computes the complexity of a position.
        // see: Computer Analysis of World Chess Champions (M. Guld & I. Bratko)
        assert(max_depth > 2);
        double complexity {};
        std::string old_best_move {};
        std::string best_move {};
        
        // the position is always the same.
        auto moves = stock.GetMoves();
        std::vector<double> evals(moves.size());
        std::vector<int> eval_perm(evals.size());
        std::iota(eval_perm.begin(), eval_perm.end(), 0);
        
        for(int d = 2; d < max_depth; d++) {
            best_move = stock.GetBestMove(d, -1);
            if (best_move != old_best_move) {
                old_best_move = best_move;
                stock.EvalMoves(evals, moves, d);
                Utils::sort_evals_perm(eval_perm, evals);
                complexity += std::abs(evals[eval_perm[0]] - evals[eval_perm[1]]);
            }
            old_best_move = best_move;
        }
        
        return complexity;
    }
    
    

    

}
// TODO: IDEA: Sharp Line generation:
// Starting from a position, select the move with highest sharpness for the opposing color, and the best response for the opposing color.
// i.e. white to play, choose the move that is sharpest for black, and then pick the best black response for that move, go on until N moves are generated.

//std::vector<std::string>
//GenerateSharpLine(int line_length, int depth)
//{
//    std::vector<std::string> line;
//    // generate a sharp line starting from the current position.
//    for (int i = 0; i < line_length; i++) {
//        
//        auto moves = GetMoves();
//        Stockfish::Move sharpest_move {};
//        double sharpest_move_sharpness {};
//        double sharpness {};
//        for (auto m : moves) {
//            // highest sharpness is the amount of blunders and bad moves in all of the moves (for now)
//            sharpness = MoveSharpness(m, depth);
//            if (sharpest_move_sharpness < sharpness) {
//                sharpest_move = m;
//                sharpest_move_sharpness = sharpness;
//            }
//        }
//        line.push_back(Utils::to_alg(pos, sharpest_move));
//        pos.do_move(sharpest_move);
//        
//        // calculate the response
//        this->send_command("position fen " + pos.fen());
//        this->send_command("go depth " +  std::to_string(depth));
//        this->read(output, "bestmove", -1);
//        
//        // super janky, poi famo un parser come si deve eh
//        auto best_response_str = output[output.size()-1];
//        auto best_response = best_response_str.substr(9, 4);
//        
//        line.push_back(Utils::long_to_alg(pos, best_response));
//        std::cout << i << ". " << sharpest_move << " " << Utils::long_to_alg(pos, best_response) << " ";
//        pos.do_move(Utils::long_alg_to_move(pos, best_response));
//    }
//    std::cout << '\n';
//    
//    return line;
//}
