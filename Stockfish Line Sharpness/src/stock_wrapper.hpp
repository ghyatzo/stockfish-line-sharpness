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

#include "position.hpp"
#include "utils.hpp"

struct StockOptions {
    int threads;
    bool showWDL;
    int multiPV;
};

class Engine : public System::Process {
public:
    Engine(const std::string &path);
    Engine(const std::string &path, int depth, std::chrono::milliseconds timeout);
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    const StockOptions& GetOptions() const { return opts_; }
    void SetOption(const std::string & optname, const std::string & optvalue);
    
    inline int Depth() const { return depth_; }
    inline int Depth(int depth) { depth_ = depth; return depth_; }

    void Start();
    void Start(const StockOptions&);
    bool Read(const std::string &expected, std::chrono::milliseconds timeout);
    inline bool Read(const std::string &expected) { return Read(expected, timeout_); }
    
    void NewGame();
    
    std::string GetBestMove(Position&);
    double Eval(Position&);
    double Eval(Stockfish::Move, Position&);
    
    template<typename cp_to_win>
    double Eval(Position & pos) {
        send_command("position fen " + pos.fen());
        send_command("go depth " +  std::to_string(depth_));
        Read("bestmove");
        
        return cp_to_win(Utils::centipawns(pos.side_to_move(), output));
    }
    template<typename cp_to_win>
    double Eval(Stockfish::Move m, Position& pos)
    {
        // evaluates the move without changing pos.
        send_command("position fen " + pos.fen() + " moves " + Utils::to_long_alg(m));
        send_command("go depth " + std::to_string(depth_));
        Read("bestmove");
        
        // measuring the first depths takes less than a ms, so we're safe.
        return cp_to_win(Utils::centipawns(~pos.side_to_move(), output));
    }
    
    void Eval(std::vector<double> &evals, const Stockfish::MoveList<Stockfish::LEGAL>&, Position&);
    std::vector<double> Eval(const Stockfish::MoveList<Stockfish::LEGAL>&, Position&);
    
public:
    std::vector<std::string> output;
private:
    std::chrono::milliseconds timeout_;
    int depth_;
    StockOptions opts_;
};

#endif /* stock_wrapper_hpp */
