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

#include "../ext/minimal-process-piping/sys_process.h"

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
    Stock(const std::string &command, const std::string &FEN);
    Stock(const Stock&) = delete;
    Stock& operator=(const Stock&) = delete;
    
    MoveList<LEGAL> GetMoves() const { return MoveList<LEGAL>(pos); }
    const StockOptions& GetOptions() const { return opts_; }
    void SetOption(const std::string & optname, const std::string & optvalue);

    void Start();
    void Start(const StockOptions&);
    bool Read(const std::string & expected, int timeout_ms = -1);
    
    void SetPosition(const std::string &FEN);
    void SetNewPosition(const std::string & FEN);
    void AdvancePosition(const std::vector<Move> &moves);
    
    std::string GetBestMove(int depth, int timeout_ms);
    double EvalPosition(int depth, int timeout_ms = -1);
    double EvalMove(Move m, int depth, int timeout_ms = -1);
    void EvalMoves(std::vector<double> &evals, const MoveList<LEGAL> &moves, int depth, int timeout_ms = -1);
    std::vector<double> EvalMoves(const MoveList<LEGAL> &moves, int depth, int timeout_ms = -1);
    
public:
    Position pos;
    std::vector<std::string> output;
private:
    StockOptions opts_;
    StateInfo si_;
};

#endif /* stock_wrapper_hpp */
