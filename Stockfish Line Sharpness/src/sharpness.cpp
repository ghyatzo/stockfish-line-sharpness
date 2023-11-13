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

static const auto WINC_THRESHOLD = Utils::lc0_cp_to_win(INACCURACY_THRESHOLD);

namespace Sharpness {
    
    MoveDist
    MoveDistributionCP(const std::vector<double> &evals, double base_eval)
    {
        double ok_moves {};
        double bad_moves {};
        double blunders {};
        for (auto e : evals) {
            double delta = std::abs(base_eval - e);
            if (delta >= BLUNDER_THRESHOLD)  { blunders++; continue; }
            if (delta <= INACCURACY_THRESHOLD) { ok_moves++; continue; }
            if (delta > INACCURACY_THRESHOLD && delta <= MISTAKE_THRESHOLD) {
                ok_moves += 0.5; bad_moves += 0.5; continue; }
            if (delta > MISTAKE_THRESHOLD) { bad_moves++; }
        }
        
        return {ok_moves, bad_moves, blunders, static_cast<double>(evals.size())};
    }
    
    MoveDist
    MoveDistributionWC(const std::vector<double> &evals, double base_eval)
    {
        double ok_moves {};
        double bad_moves {};
        for (auto e : evals) {
            double delta = std::abs(base_eval - e);
            if (delta <= WINC_THRESHOLD) { ok_moves++; }
            else { bad_moves++; }
        }
        
        return {ok_moves, bad_moves, static_cast<double>(evals.size())};
    }
    
    MoveDist
    ComputePosition(Engine &engine, Position& pos)
    {
        double base_eval = engine.Eval(pos);
        auto evals = engine.Eval(pos.GetMoves(), pos);
        
        return MoveDistributionCP(evals, base_eval);
    }
    
    // TODO: these still needs work.
    MoveDist ComputeMove(Stockfish::Move m, Engine &engine, Position& pos)
    {
        // how sharp is this move: playing this move generates a position, how sharp is that position?
        // how sharp is the resulting position for the adversary?
        pos.DoMove(m);
        auto move_dist = ComputePosition(engine, pos);
        pos.UndoMove(m);
        
        return move_dist;
    }
    
    double Ratio(const MoveDist &movedist)
    {
        // old version
        // return (movedist.blunders + movedist.bad) / movedist.total_moves;
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
    
    // TODO: Make a sharpness metric that compares the WDL changes for the moves in a position.
    
    double Complexity(Engine& engine, Position& pos, int max_depth)
    {
        // computes the complexity of a position.
        // see: Computer Analysis of World Chess Champions (M. Guld & I. Bratko)
        assert(max_depth > 2);
        double complexity {};
        std::string old_best_move {};
        std::string best_move {};
        
        // the position is always the same.
        auto moves = pos.GetMoves();
        std::vector<double> evals(moves.size());
        std::vector<int> eval_perm(evals.size());
        std::iota(eval_perm.begin(), eval_perm.end(), 0);
        
        for(int d = 2; d < max_depth; d++) {
            best_move = engine.GetBestMove(pos);
            if (best_move != old_best_move) {
                old_best_move = best_move;
                engine.Eval(evals, moves, pos);
                Utils::sort_evals_perm(eval_perm, evals);
                complexity += std::abs(evals[eval_perm[0]] - evals[eval_perm[1]]);
            }
            old_best_move = best_move;
        }
        
        return complexity;
    }
    
    
    // TODO: IDEA: Sharp Line generation:
    // Starting from a position, select the move with highest sharpness for the opposing color, and the best response for the opposing color.
    // i.e. white to play, choose the move that is sharpest for black, and then pick the best black response for that move, go on until N moves are generated.
    
    std::vector<std::string>
    GenerateLine(size_t line_length, Position& pos, Engine& engine)
    {
        std::vector<std::string> line;
        // generate a sharp line starting from the current position.
        
        for (int i = 0; i < line_length; i++) {
            
            Stockfish::Move sharpest_move {};
            double sharpest_move_sharpness {};
            double sharpness {};
            auto moves = pos.GetMoves();
            auto base_eval = engine.Eval(pos);
            
            for (auto m : moves) {
                // We have to filter the moves that do not throw the game.
                // Otherwise, THIS DOES NOT WORK, When computing the sharpest move, this means that we will do a move that maximises the sharpness of the position that follows that move.
                // The way we compute the sharpness is by comparing bad and good moves, therefore, a move that makes almost all moves bad for the opponent is very sharp.
                // But if I hang a piece, every move the opponent does that does not take the hanging piece is considered bad, resulting in a very high sharpness.
                auto move_eval = engine.Eval(m, pos);
                if (abs(base_eval-move_eval) >= INACCURACY_THRESHOLD) continue;
                
                sharpness = Ratio(ComputeMove(m, engine, pos));
                if (sharpest_move_sharpness < sharpness) {
                    sharpest_move = m;
                    sharpest_move_sharpness = sharpness;
                }
            }

            line.push_back(Utils::to_alg(pos, sharpest_move));
            std::cout << i << ". " << line.back() << " ";
            pos.DoMove(sharpest_move);
            
            // calculate the response
            auto best_response = engine.GetBestMove(pos);
            line.push_back(Utils::long_to_alg(pos, best_response));
            std::cout << line.back() << std::endl;
            pos.DoMove(Utils::long_alg_to_move(pos, best_response));
        }
        std::cout << '\n';
        
        return line;
    }

}
