//
//  stock_wrapper.hpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 09/10/2023.
//

#ifndef stock_wrapper_hpp
#define stock_wrapper_hpp

#include <stdio.h>
#include <string>
#include "utils.hpp"
#include "sys_process.h"
#include "mini_stock/bitboard.h"
#include "mini_stock/position.h"
#include "mini_stock/movegen.h"

using namespace Stockfish;

class Stock : public System::Process {
public:
    Stock(const std::string command);
    Stock(const Stock&) = delete;
    Stock& operator=(const Stock&) = delete;
    
    StateInfo getState() { return si_; }
    
    void Start();
    bool Read(const std::string &expected, int timeout_ms = -1);
    void SetNewPosition(const std::string &FEN);
    MoveList<LEGAL> GetMoves();
    double EvalPosition(int depth, int timeout_ms = -1);
    double EvalMove(Move m, int depth, int timeout_ms = -1);
    std::vector<double> EvalMoves(MoveList<LEGAL> &moves, int depth, int timeout_ms = -1);
    
    std::tuple<double, double>
    ComputeSharpness(const std::vector<double> &evals, double base_eval,
                     double bad_th, double ok_th, double dont_care_th);
    
    double
    PositionSharpness(int depth, double bad_th, double ok_th, double dont_care_th);
public:
    Position pos;
    std::vector<std::string> output;
private:
    StateInfo si_;
};

#endif /* stock_wrapper_hpp */
