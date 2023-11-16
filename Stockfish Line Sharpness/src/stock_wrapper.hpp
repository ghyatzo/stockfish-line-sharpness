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

struct EngineOptions {
    int threads;
    bool showWDL;
    int multiPV;
};

class Engine : public System::Process {
public:
    Engine(const std::string &path) : System::Process(path), output_() {}
    Engine(const std::string &path, int depth, std::chrono::milliseconds timeout)
        : System::Process(path), output_(), depth_(depth), timeout_(timeout) {}
    
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    const EngineOptions& GetOptions() const { return opts_; }
    void SetOption(const std::string & optname, const std::string & optvalue);
    
    inline int Depth() const {
        return depth_;
    }
    inline int Depth(int depth) {
        return depth_ = depth;
    }
    inline int Timeout() const {
        return (int)timeout_.count();
    }
    inline int Timeout(std::chrono::milliseconds timeout) {
        timeout_ = timeout;
        return (int)timeout_.count();
    }

    void Start(const EngineOptions&);
    inline void Start() { Start(opts_); };
    inline bool Read(const std::string &expected, std::chrono::milliseconds timeout) {
        return read(output_, expected, (int)timeout.count());
    };
    inline bool Read(const std::string &expected){
        return read(output_, expected, (int)timeout_.count());
    }
    
    std::string GetBestMove(const Position&);
    double Eval(const Position&);
    double EvalMove(Stockfish::Move, Position&);
    
    std::vector<double> EvalMoves(std::vector<double> &evals,
                                  const Stockfish::MoveList<Stockfish::LEGAL>&, Position&);
    std::vector<double> EvalMoves(const Stockfish::MoveList<Stockfish::LEGAL>&, Position&);
    
//    template<typename F = std::identity>
//    double Eval(Position & pos, F && f = {}) {
//        send_command("position fen " + pos.fen());
//        send_command("go depth " +  std::to_string(depth_));
//        Read("bestmove");
//        
//        return f(Utils::centipawns(pos.side_to_move(), output));
//    }
//    template<typename F = std::identity>
//    double Eval(Stockfish::Move m, Position& pos, F && f = {})
//    {
//        // evaluates the move without changing pos.
//        send_command("position fen " + pos.fen() + " moves " + Utils::to_long_alg(m));
//        send_command("go depth " + std::to_string(depth_));
//        Read("bestmove");
//        
//        // measuring the first depths takes less than a ms, so we're safe.
//        return f(Utils::centipawns(~pos.side_to_move(), output));
//    }
//    
//    template<typename F = std::identity>
//    std::vector<double> Eval(const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos, F && f = {})
//    {
//        std::vector<double> evals;
//        evals.reserve(moves.size());
//        for (const auto m: moves) {
//            evals.emplace_back(Eval(m, pos, f));
//        }
//        return evals;
//    }
//    
//    // In-place version of the function above.
//    template<typename F = std::identity>
//    void Eval(std::vector<double> &evals, const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos, F && f = {})
//    {
//        evals.resize(moves.size());
//        for (int count {}; const auto m: moves) {
//            evals.emplace(evals.begin()+count, Eval(m, pos, f));
//        }
//    }
    
private:
    std::vector<std::string> output_;
    std::chrono::milliseconds timeout_ {-1};
    int depth_ {15};
    EngineOptions opts_
    {
        .multiPV = 1,
        .showWDL = true,
        .threads = 4
    };
};

#endif /* stock_wrapper_hpp */
