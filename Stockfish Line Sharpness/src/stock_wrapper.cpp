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

void Engine::Start(const EngineOptions &opts)
{
    // we don't need to pass any argument, just call the executable.
    const char * const argv[] = { command_.c_str(), NULL };
    start(argv);
    
    // start uci mode, check for "uciok", and the various options to parse.
    send_command("uci");
    if (Read("uciok") == false)
        throw std::runtime_error("could not set stockfish to uci mode");
    
    SetOption("UCI_showWDL", opts.showWDL ? "true" : "false");
    SetOption("Threads", std::to_string(opts.threads));
    SetOption("MultiPV", std::to_string(opts.multiPV));

    send_command("ucinewgame");
    send_command("isready");
    
    // wait for the stockfish to be ready
    Read("readyok");
}

void Engine::SetOption(const std::string & optname, const std::string & optvalue)
{
    if (optname == "UCI_showWDL" ) opts_.showWDL = optvalue == "true" ? true : false;
    if (optname == "Threads" ) opts_.threads = std::stoi(optvalue);
    if (optname == "MultiPV" ) opts_.multiPV = std::stoi(optvalue);
        
    send_command("setoption " + optname + " value " + optvalue);
}

std::string Engine::GetBestMove(const Position& pos)
{
    send_command("position fen " + pos.fen());
    send_command("go depth " + std::to_string(depth_));
    Read("bestmove");
    
    return Utils::parse_best_move(output_);
}

// evaluates a position
double Engine::Eval(const Position& pos)
{
    send_command("position fen " + pos.fen());
    send_command("go depth " +  std::to_string(depth_));
    Read("bestmove");
    
    return Utils::lc0_cp_to_win(Utils::centipawns(pos.side_to_move(), output_)*100);
}

//evaluates a move in a given position, by evaluating the position after the move.
double Engine::EvalMove(Stockfish::Move m, Position& pos)
{
    // evaluates the move without changing pos.
    auto longm = Utils::to_long_alg(m);
    if ( longm == "e8h8" ) {
        longm = "e8g8";
    } else if (longm == "e8a8") {
        longm = "e8b8";
    } else if (longm == "e1h1") {
        longm = "e1g1";
    } else if (longm == "e1a1") {
        longm = "e1b1";
    }
    send_command("position fen " + pos.fen() + " moves " + longm);
    send_command("go depth " + std::to_string(depth_));
    Read("bestmove");
    
    // measuring the first depths takes less than a ms, so we're safe.
    return Utils::lc0_cp_to_win(Utils::centipawns(~pos.side_to_move(), output_)*100);
}

// In-place version of the function above.
std::vector<double> Engine::EvalMoves(std::vector<double> &evals,
                                      const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos)
{
    evals.resize(moves.size());
    auto move = moves.begin();
    for ( int idx {}; idx < moves.size(); idx++ ) {
        evals[idx] = EvalMove(*(move + idx), pos);
    }
    return evals;
}

// evaluates a list of legal moves in a single position.
std::vector<double> Engine::EvalMoves(const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos)
{
    std::vector<double> evals;
    evals.reserve(moves.size());
    return EvalMoves(evals, moves, pos);
}




