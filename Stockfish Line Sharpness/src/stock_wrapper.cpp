//
//  stock_wrapper.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 09/10/2023.
//

#include <iostream>
#include <tuple>
#include <numeric>

#include "stock_wrapper.hpp"
#include "utils.hpp"
#include "fen.hpp"

// default to the starting position, if not FEN is passed.
Engine::Engine(const std::string &path)
: System::Process(path), output(), opts_(), depth_(15), timeout_(-1)
{
    // default options
    opts_.multiPV = 1;
    opts_.showWDL = true;
    opts_.threads = 4;
}
Engine::Engine(const std::string &path, int depth, std::chrono::milliseconds timeout)
: System::Process(path), output(), opts_(), depth_(depth), timeout_(timeout)
{
    // default options
    opts_.multiPV = 1;
    opts_.showWDL = true;
    opts_.threads = 4;
}

void Engine::Start() { Start(opts_); }
void Engine::Start(const StockOptions &opts)
{
    // we don't need to pass any argument, just call the executable.
    const char * const argv[] = { command_.c_str(), NULL };
    start(argv);
    
    // start uci mode, check for "uciok", and the various options to parse.
    send_command("uci");
    if (read(output, "uciok") == false)
        throw std::runtime_error("could not set stockfish to uci mode");
    
    SetOption("UCI_showWDL", opts.showWDL ? "true" : "false");
    SetOption("Threads", std::to_string(opts.threads));
    SetOption("MultiPV", std::to_string(opts.multiPV));

    send_command("ucinewgame");
    send_command("isready");
    
    // wait for the stockfish to be ready
    read(output, "readyok");
}

void Engine::SetOption(const std::string & optname, const std::string & optvalue)
{
    if (optname == "UCI_showWDL" ) opts_.showWDL = optvalue == "true" ? true : false;
    if (optname == "Threads" ) opts_.threads = std::stoi(optvalue);
    if (optname == "MultiPV" ) opts_.multiPV = std::stoi(optvalue);
        
    send_command("setoption " + optname + " value " + optvalue);
}

bool Engine::Read(const std::string &expected, std::chrono::milliseconds timeout)
{
    return read(output, expected, int(timeout.count()));
}

void Engine::NewGame()
{
    send_command("ucinewgame");
    send_command("isready");
    
    // wait for stockfish to be ready
    read(output, "readyok");
}

std::string Engine::GetBestMove(Position& pos)
{
    send_command("position fen " + pos.fen());
    send_command("go depth " + std::to_string(depth_));
    Read("bestmove");
    
    return Utils::parse_best_move(output);
}

//// evaluates a position
//double Engine::Eval(Position& pos)
//{
//    send_command("position fen " + pos.fen());
//    send_command("go depth " +  std::to_string(depth_));
//    Read("bestmove");
//    
//    return Utils::centipawns(pos.side_to_move(), output);
//}
//
////evaluates a move in a given position, by evaluating the position after the move.
//double Engine::Eval(Stockfish::Move m, Position& pos)
//{
//    // evaluates the move without changing pos.
//    send_command("position fen " + pos.fen() + " moves " + Utils::to_long_alg(m));
//    send_command("go depth " + std::to_string(depth_));
//    Read("bestmove");
//    
//    // measuring the first depths takes less than a ms, so we're safe.
//    return Utils::centipawns(~pos.side_to_move(), output);
//}
//
//// evaluates a list of legal moves in a single position.
//std::vector<double> Engine::Eval(const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos)
//{
//    std::vector<double> evals;
//    evals.reserve(moves.size());
//    for (int count {}; const auto m: moves) {
//        evals.emplace_back(Eval(m, pos));
////        PROGRESS_BAR(count++);
//    }
//    return evals;
//}
//
//// In-place version of the function above.
//void Engine::Eval(std::vector<double> &evals, const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos)
//{
//    evals.resize(moves.size());
//    for (int count {}; const auto m: moves) {
//        evals.emplace(evals.begin()+count, Eval(m, pos));
////        PROGRESS_BAR(count++);
//    }
//}



