//
//  stock_wrapper.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 09/10/2023.
//

#include "stock_wrapper.hpp"
#include <iostream>
#include <tuple>

Stock::Stock(const std::string command)
: System::Process {command}, si_{}, pos {}, output {}
{
    Stockfish::Bitboards::init();
    Stockfish::Position::init();
}

void Stock::Start()
{
    // we don't need to pass any argument, just call the executable.
    const char * const argv[] = { this->command_.c_str(), NULL };
    this->start(argv);
    
    // start uci mode, check for "uciok", and the various options to parse.
    this->send_command("uci");
    if (this->read(output, "uciok") == false)
        throw std::runtime_error("could not set stockfish to uci mode");
}

bool Stock::Read(const std::string &expected, int timeout_ms)
{
    return this->read(output, expected, timeout_ms);
}


void Stock::SetNewPosition(const std::string &FEN)
{
    // setup a new game with : "ucinewgame"
    // followup with a "isready" and wait for a "readyok"
    if (!this->is_alive()) {
        std::cout << "Please start stockfish first" << std::endl;
        return;
    }
    this->send_command("ucinewgame");
    this->send_command("isready");
    
    // wait for the stockfish to be ready
    this->read(output, "readyok");
    pos.set(FEN, false, &si_, nullptr);
}

double Stock::EvalPosition(int depth, int timeout_ms)
{
    this->send_command("position fen " + pos.fen());
    this->send_command("go depth " +  std::to_string(depth));
    this->read(output, "bestmove", timeout_ms);
    
    return Utils::centipawns(pos.side_to_move(), output[output.size()-2]);
}

double Stock::EvalMove(Move m, int depth, int timeout_ms)
{
    // evaluates the move without changing pos.
    this->send_command("position fen " + pos.fen() + " moves " + Utils::to_long_alg(m));
    this->send_command("go depth " + std::to_string(depth));
    this->read(output, "bestmove", timeout_ms);
    
    // measuring the first depths takes less than a ms, so we're safe.
    return Utils::centipawns(~pos.side_to_move(), output[output.size()-2]);
}

MoveList<LEGAL> Stock::GetMoves() { return MoveList<LEGAL>(pos); }

std::vector<double> Stock::EvalMoves(MoveList<LEGAL> &moves, int depth, int timeout_ms)
{
    std::vector<double> evals;
    evals.reserve(moves.size());
    auto count = 0;
    std::cout << "[" << std::string(moves.size() - count, '.') << "]" << std::flush;
    for (auto i = moves.begin(); i < moves.end(); i++) {
        std::cout << "\x1b[1K\x1b[G" << "[" << std::string(count, 'o') << std::string(moves.size() - count, '.') << "] " << count << "/" << moves.size() << std::flush;
        Move m {i->move};
//        std::cout << to_alg(pos, m) << " (" << to_long_alg(m) << ")";
        evals.emplace(evals.end(), EvalMove(m, depth, timeout_ms));
//        evals.emplace_back(EvalMove(m, depth, timeout_m;s));
        count++;
    }
    std::cout << "\x1b[1K\x1b[G" << "[" << std::string(moves.size(), 'o') << "]" << std::endl;
    
    
    return evals;
}

std::tuple<double, double>
Stock::ComputeSharpness(const std::vector<double> &evals, double base_eval,
                        double bad_th, double ok_th, double dont_care_th)
{
    double ok_moves = 0;
    double bad_moves = 0;
    for (auto e : evals) {
        // if you blunder mate in 1 but still have 10+ in the evaluation, do you even care?
        if (std::abs(e) >= dont_care_th) { ok_moves++; continue; }
        
        double delta = std::abs(base_eval - e);
        if (delta <= ok_th) { ok_moves++; continue; }
        if (delta >= bad_th) { bad_moves++; }
    }
    
    return std::make_tuple(ok_moves, bad_moves);
    
    // print the ratio
//    if (!bad_moves) std::cout << "There are no blunders in the position." << std::endl;
//    if (!ok_moves) std::cout << "There are no good moves in the position." << std::endl;
//    else std::cout << "The position has a sharpness ratio of: " << "(blunders/ok) " << bad_moves <<
//        "/" << ok_moves << "=" << bad_moves/ok_moves << std::endl;
}

double
Stock::PositionSharpness(int depth, double bad_th, double ok_th, double dont_care_th)
{
    double base_eval = EvalPosition(depth, -1);
    auto moves = GetMoves();
    auto evals = EvalMoves(moves, depth, -1);
    
    auto [ok_moves, bad_moves] = ComputeSharpness(evals, base_eval, bad_th, ok_th, dont_care_th);
    return bad_moves / (ok_moves+bad_moves);
}

//Stock::EvalLine(const std::string &startposFEN, const MoveList<LEGAL> &moves,
//                int depth, int timeout_ms)
//{
//    
//}


