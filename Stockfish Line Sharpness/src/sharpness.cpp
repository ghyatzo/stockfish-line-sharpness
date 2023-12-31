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

static const auto WINC_THRESHOLD = std::abs(Utils::lc0_cp_to_win(INACCURACY_THRESHOLD*100));
static const auto WINC_BLUNDER_THRESHOLD = std::abs(Utils::lc0_cp_to_win(BLUNDER_THRESHOLD*100));

namespace Sharpness {
    
    double TotalVar( const std::vector<double> &evals, double base_eval, Stockfish::Color col) {
        // Computes the accumulated differences between two succesive (sorted) move evaluations,
        // up until the first bad move (included).
        double tv {};
        auto sorted_perm = Utils::sort_evals_perm(evals, col);
        if (sorted_perm.size() < 2) return 0;
        
        int i = 0;
        double count = 0;
        while ( std::abs( base_eval - evals[sorted_perm[i]] ) < WINC_THRESHOLD ) {
            tv += std::abs( evals[sorted_perm[i]] - evals[sorted_perm[i+1]] );
            
            count++;
            i++;
            if ( i >= sorted_perm.size() - 1) break;
        }
        return tv/(count);
    }
    
    double
    ComputePosition(Engine &engine, Position& pos)
    {
        double base_eval = engine.Eval(pos);
        auto evals = engine.EvalMoves(pos.GetMoves(), pos);
        
        return TotalVar(evals, base_eval, pos.side_to_move());
    }
    
    // TODO: these still needs work.
    double ComputeMove(Stockfish::Move m, Engine &engine, Position& pos)
    {
        // how sharp is this move: playing this move generates a position, how sharp is that position?
        // how sharp is the resulting position for the adversary?
        pos.DoMove(m);
        auto total_var = ComputePosition(engine, pos);
        pos.UndoMove(m);
        
        return total_var;
    }
    
    // TODO: Make a sharpness metric that compares the WDL changes for the moves in a position.
    
    double Complexity(Engine& engine, Position& pos, int max_depth)
    {
        // computes the complexity of a position.
        // based on: Computer Analysis of World Chess Champions (M. Guld & I. Bratko)
        assert(max_depth > 2);
        auto current_depth = engine.Depth();
        double complexity {};
        std::string old_best_move {};
        std::string best_move {};
        
        // the position is always the same.
        auto moves = pos.GetMoves();
        std::vector<double> evals(moves.size());
        std::vector<int> eval_perm(evals.size());
        std::iota(eval_perm.begin(), eval_perm.end(), 0);
        int change_of_mind {};
        
        for(int d = 2; d < max_depth; d++) {
            engine.Depth(d);
            best_move = engine.GetBestMove(pos);
            if (best_move != old_best_move) {
                change_of_mind++;
                old_best_move = best_move;
                engine.EvalMoves(evals, moves, pos);
                Utils::sort_evals_perm(eval_perm, evals, pos.side_to_move());
                auto delta = std::abs(evals[eval_perm[0]] - evals[eval_perm[1]]);
                complexity += delta;
            }
            old_best_move = best_move;
        }
        
        engine.Depth(current_depth);
        return change_of_mind;
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
            double sharpest_move_sharpness { -std::numeric_limits<double>::infinity() };
            double sharpness {};
            auto moves = pos.GetMoves();
            auto base_eval = engine.Eval(pos);
            
            for (int count{}; auto m : moves) {
                PROGRESS_BAR(count++, moves.size());
                // We have to filter the moves that do not throw the game.
                // Otherwise, THIS DOES NOT WORK, When computing the sharpest move, this means that we will do a move that maximises the sharpness of the position that follows that move.
                // The way we compute the sharpness is by comparing bad and good moves, therefore, a move that makes almost all moves bad for the opponent is very sharp.
                // But if I hang a piece, every move the opponent does that does not take the hanging piece is considered bad, resulting in a very high sharpness.
                auto move_eval = engine.EvalMove(m, pos);
                auto delta = abs(base_eval-move_eval);
                if (delta >= WINC_THRESHOLD) continue;
                
                // Only using the sharpness could lead to very bad moves, maybe weight it with how bad a move could be compared to how sharp it is.
                
                sharpness = ComputeMove(m, engine, pos);
                auto overall_score = sharpness - delta*0.5;
                if (sharpest_move_sharpness < overall_score) {
                    sharpest_move = m;
                    sharpest_move_sharpness = overall_score;
                }
            }

            line.push_back(Utils::to_alg(pos, sharpest_move));
            if (pos.side_to_move() == Stockfish::BLACK)
                std::cout << i << ". ... " << line.back() << std::endl;
            else
                std::cout << i << ". " << line.back() << " ";
            pos.DoMove(sharpest_move);
            
            // calculate the response
            auto best_response = engine.GetBestMove(pos);
            line.push_back(Utils::long_to_alg(pos, best_response));
            
            if (pos.side_to_move() == Stockfish::WHITE)
                std::cout << i+1 << ". " << line.back() << "\n";
            else
                std::cout << line.back() << std::endl;

            pos.DoMove(Utils::long_alg_to_move(pos, best_response));
        }
        std::cout << '\n';
        
        return line;
    }

}
