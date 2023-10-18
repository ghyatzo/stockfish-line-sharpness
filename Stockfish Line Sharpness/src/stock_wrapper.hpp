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
#include "../ext/sys_process.h"
#include "mini_stock/bitboard.h"
#include "mini_stock/position.h"
#include "mini_stock/movegen.h"

using namespace Stockfish;
struct StockOptions {
    int threads;
    bool showWDL;
    int multiPV;
};

class Stock : public System::Process {
public:
    Stock(const std::string command);
    Stock(const Stock&) = delete;
    Stock& operator=(const Stock&) = delete;
    
    void Start();
    void Start(const StockOptions & opts);
    void SetOption(const std::string & optname, const std::string & optvalue);
    bool Read(const std::string &expected, int timeout_ms = -1);
    void SetNewPosition(const std::string &FEN);
    MoveList<LEGAL> GetMoves();
    double EvalPosition(int depth, int timeout_ms = -1);
    double EvalMove(Move m, int depth, int timeout_ms = -1);
    std::vector<double> EvalMoves(MoveList<LEGAL> &moves, int depth, int timeout_ms = -1);
    
    std::tuple<double, double, double>
    ComputeSharpness(const std::vector<double> &evals, double base_eval,
                     double blunders_th, double bad_th, double ok_th);
    
    double
    MoveSharpness(Stockfish::Move m, int depth,
                     double blunder_th, double bad_th, double ok_th);
    
    std::tuple<double, double>
    PositionSharpness(int depth, double blunders_th, double bad_th, double ok_th);
    
    std::vector<std::string>
    GenerateSharpLine(int line_length, int depth, double blund_th, double bad_th, double ok_th);
public:
    Position pos;
    std::vector<std::string> output;
private:
    StockOptions opts_;
    StateInfo si_;
};

#endif /* stock_wrapper_hpp */
