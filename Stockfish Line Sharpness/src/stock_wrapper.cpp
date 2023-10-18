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
#include "../ext/dbg.h"

Stock::Stock(const std::string command)
: System::Process {command}, si_{}, pos {}, output {}, opts_{}
{
    Stockfish::Bitboards::init();
    Stockfish::Position::init();
    
    // default options
    this->opts_.multiPV = 1;
    this->opts_.showWDL = true;
    this->opts_.threads = 4;
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
    
    // set up various options
    SetOption("UCI_showWDL", this->opts_.showWDL ? "true" : "false");
    SetOption("Threads", std::to_string(this->opts_.threads));
    SetOption("MultiPV", std::to_string(this->opts_.multiPV));
}
void Stock::Start(const StockOptions &opts)
{
    // we don't need to pass any argument, just call the executable.
    const char * const argv[] = { this->command_.c_str(), NULL };
    this->start(argv);
    
    // start uci mode, check for "uciok", and the various options to parse.
    this->send_command("uci");
    if (this->read(output, "uciok") == false)
        throw std::runtime_error("could not set stockfish to uci mode");
    
    // set up various options
    SetOption("UCI_showWDL", opts.showWDL ? "true" : "false");
    SetOption("Threads", std::to_string(opts.threads));
    SetOption("MultiPV", std::to_string(opts.multiPV));
}

void Stock::SetOption(const std::string & optname, const std::string & optvalue)
{
    if (optname == "UCI_showWDL" ) this->opts_.showWDL = optvalue == "true" ? true : false;
    if (optname == "Threads" ) this->opts_.threads = std::stoi(optvalue);
    if (optname == "MultiPV" ) this->opts_.multiPV = std::stoi(optvalue);
        
    this->send_command("setoption " + optname + " value " + optvalue);
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
//    std::cout << "\x1b[1K\x1b[G" << "[" << std::string(moves.size(), 'o') << "]" << std::endl;
    std::cout << "\x1b[1K\x1b[G";
    
    
    return evals;
}

std::tuple<double, double, double>
Stock::ComputeSharpness(const std::vector<double> &evals, double base_eval,
                        double blunder_th, double bad_th, double ok_th)
{
    double ok_moves = 0;
    double bad_moves = 0;
    double blunders = 0;
    for (auto e : evals) {
        // if you blunder mate in 1 but still have 10+ in the evaluation, do you even care?
//        if (std::abs(e) >= dont_care_th) { ok_moves++; continue; }
        double delta = std::abs(base_eval - e);
        if (delta >= blunder_th) { blunders++; continue; }
        
        if (delta <= ok_th) { ok_moves++; continue; }
        if (delta > ok_th && delta <= bad_th) { ok_moves += 0.5; bad_moves += 0.5; continue; }
        if (delta > bad_th) { bad_moves++; }
    }
    
    return std::make_tuple(ok_moves, bad_moves, blunders);
}

double
Stock::MoveSharpness(Stockfish::Move m, int depth,
                        double blunder_th, double bad_th, double ok_th)
{
    // how sharp is this move: playing this move generates a position, how sharp is that position?
    // how sharp is the resulting position for the adversary?
    pos.do_move(m);
    auto base_eval = EvalPosition(depth);
    auto moves = GetMoves();
    auto evals = EvalMoves(moves, depth);
    auto [ok, bad, blunders] = ComputeSharpness(evals, base_eval, blunder_th, bad_th, ok_th);
    pos.undo_move(m);
    return (bad+blunders)/moves.size();
}
// TODO: IDEA: compute the highest jump between all non blunder move evaluation.
// from the best to the worst. per ora fa cagare.
//std::tuple<double, double, double>
//Stock::ComputeSharpness2(const std::vector<double> &evals, double base_eval,
//                        double blunder_th, double bad_th, double ok_th, double dont_care_th)
//{
//    double max_jump = 0;
//    double blunders = 0;
//    double max_jump_idx = 1;
//    if (evals.size() == 1) return std::make_tuple(0, 0, 1); // for now.
//    std::vector<int> idx_perm(evals.size());
//    std::iota(idx_perm.begin(), idx_perm.end(), 0);
//    std::sort(idx_perm.begin(), idx_perm.end(), [&](int a, int b){
//        double delta_a = std::abs(base_eval - evals[a]);
//        double delta_b = std::abs(base_eval - evals[b]);
//        return delta_a < delta_b;
//    });
//    for (int i = 1; i < idx_perm.size(); i++) {
//        double delta = std::abs(base_eval - evals[idx_perm[i]]);
//        if (delta >= blunder_th) { blunders++; continue; }
//        auto jump = std::abs(evals[idx_perm[i]] - evals[idx_perm[i-1]]);
//        if (jump > max_jump) {
//            max_jump = std::max(max_jump, jump);
//            max_jump_idx = idx_perm[i];
//        }
//    }
//    
//    return std::make_tuple(max_jump, max_jump_idx, blunders);
//}

std::tuple<double, double>
Stock::PositionSharpness(int depth, double blund_th, double bad_th, double ok_th)
{
    double base_eval = EvalPosition(depth, -1);
    auto moves = GetMoves();
    auto evals = EvalMoves(moves, depth, -1);
    
    auto [ok_moves, bad_moves, blunders] = ComputeSharpness(evals, base_eval, blund_th, bad_th, ok_th);
    auto ratio = bad_moves / (ok_moves+bad_moves);
    return std::make_tuple(ratio, blunders);
}

// TODO: IDEA: Sharp Line generation:
// Starting from a position, select the move with highest sharpness for the opposing color, and the best response for the opposing color.
// i.e. white to play, choose the move that is sharpest for black, and then pick the best black response for that move, go on until N moves are generated.

std::vector<std::string>
Stock::GenerateSharpLine(int line_length, int depth, double blund_th, double bad_th, double ok_th)
{
    std::vector<std::string> line;
    // generate a sharp line starting from the current position.
    for (int i = 0; i < line_length; i++) {
        
        auto moves = GetMoves();
        Move sharpest_move {};
        double sharpest_move_sharpness {};
        double sharpness {};
        for (auto m : moves) {
            // highest sharpness is the amount of blunders and bad moves in all of the moves (for now)
            sharpness = MoveSharpness(m, depth, blund_th, bad_th, ok_th);
            if (sharpest_move_sharpness < sharpness) {
                sharpest_move = m;
                sharpest_move_sharpness = sharpness;
            }
        }
        line.push_back(Utils::to_alg(pos, sharpest_move));
        pos.do_move(sharpest_move);
        
        // calculate the response
        this->send_command("position fen " + pos.fen());
        this->send_command("go depth " +  std::to_string(depth));
        this->read(output, "bestmove", -1);
        
        // super janky, poi famo un parser come si deve eh
        auto best_response_str = output[output.size()-1];
        auto best_response = best_response_str.substr(9, 4);
        
        line.push_back(Utils::long_to_alg(pos, best_response));
        std::cout << i << ". " << sharpest_move << " " << Utils::long_to_alg(pos, best_response) << " ";
        pos.do_move(Utils::long_alg_to_move(pos, best_response));
    }
    std::cout << '\n';
    
    return line;
}


