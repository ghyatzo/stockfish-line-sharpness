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
Stock::Stock(const std::string &command,
             const std::string &FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
: System::Process(command), si_(), pos(), output(), opts_()
{
    Stockfish::Bitboards::init();
    Stockfish::Position::init();
    
    SetPosition(FEN);
    
    // default options
    this->opts_.multiPV = 1;
    this->opts_.showWDL = true;
    this->opts_.threads = 4;
    
}

void Stock::Start() { Start(opts_); }
void Stock::Start(const StockOptions &opts)
{
    // we don't need to pass any argument, just call the executable.
    const char * const argv[] = { this->command_.c_str(), NULL };
    this->start(argv);
    
    // start uci mode, check for "uciok", and the various options to parse.
    this->send_command("uci");
    if (this->read(output, "uciok") == false)
        throw std::runtime_error("could not set stockfish to uci mode");
    
    SetOption("UCI_showWDL", opts.showWDL ? "true" : "false");
    SetOption("Threads", std::to_string(opts.threads));
    SetOption("MultiPV", std::to_string(opts.multiPV));

    this->send_command("ucinewgame");
    this->send_command("isready");
    
    // wait for the stockfish to be ready
    this->read(output, "readyok");
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

void Stock::SetPosition(const std::string &FEN)
{
    if (checkFEN(FEN.c_str()) < 0)
        throw std::runtime_error("Please enter a valid FEN string");
    pos.set(FEN, false, &si_, nullptr);
}

void Stock::SetNewPosition(const std::string &FEN)
{
    this->send_command("ucinewgame");
    this->send_command("isready");
    
    // wait for stockfish to be ready
    this->read(output, "readyok");
    SetPosition(FEN);
}

void Stock::AdvancePosition(const std::vector<Move> &moves)
{
    for (const auto mm : moves) { pos.do_move(mm); }
}

std::string Stock::GetBestMove(int depth, int timeout_ms)
{
    this->send_command("position fen " + pos.fen());
    this->send_command("go depth " + std::to_string(depth));
    Read("bestmove", timeout_ms);
    
    return Utils::parse_best_move(output);
}

double Stock::EvalPosition(int depth, int timeout_ms)
{
    this->send_command("position fen " + pos.fen());
    this->send_command("go depth " +  std::to_string(depth));
    Read("bestmove", timeout_ms);
    
    return Utils::centipawns(pos.side_to_move(), output[output.size()-2]);
}

double Stock::EvalMove(Move m, int depth, int timeout_ms)
{
    // evaluates the move without changing pos.
    this->send_command("position fen " + pos.fen() + " moves " + Utils::to_long_alg(m));
    this->send_command("go depth " + std::to_string(depth));
    Read("bestmove", timeout_ms);
    
    // measuring the first depths takes less than a ms, so we're safe.
    return Utils::centipawns(~pos.side_to_move(), output[output.size()-2]);
}

void Stock::EvalMoves(std::vector<double> &evals, const MoveList<LEGAL> &moves, int depth, int timeout_ms)
{
    evals.resize(moves.size());
    for (int count {}; const auto m: moves) {
        evals.emplace(evals.begin()+count, EvalMove(m, depth, timeout_ms));
        PROGRESS_BAR(count, moves.size());
        count++;
    }
}

std::vector<double> Stock::EvalMoves(const MoveList<LEGAL> &moves, int depth, int timeout_ms)
{
    std::vector<double> evals;
    evals.reserve(moves.size());
    for (int count {}; const auto m: moves) {
        evals.emplace_back(EvalMove(m, depth, timeout_ms));
        PROGRESS_BAR(count, moves.size());
        count++;
    }
    return evals;
}

//// TODO: IDEA: Sharp Line generation:
//// Starting from a position, select the move with highest sharpness for the opposing color, and the best response for the opposing color.
//// i.e. white to play, choose the move that is sharpest for black, and then pick the best black response for that move, go on until N moves are generated.
//
//std::vector<std::string>
//Stock::GenerateSharpLine(int line_length, int depth, double blund_th, double bad_th, double ok_th)
//{
//    std::vector<std::string> line;
//    // generate a sharp line starting from the current position.
//    for (int i = 0; i < line_length; i++) {
//        
//        auto moves = GetMoves();
//        Move sharpest_move {};
//        double sharpest_move_sharpness {};
//        double sharpness {};
//        for (auto m : moves) {
//            // highest sharpness is the amount of blunders and bad moves in all of the moves (for now)
//            sharpness = MoveSharpness(m, depth, blund_th, bad_th, ok_th);
//            if (sharpest_move_sharpness < sharpness) {
//                sharpest_move = m;
//                sharpest_move_sharpness = sharpness;
//            }
//        }
//        line.push_back(Utils::to_alg(pos, sharpest_move));
//        pos.do_move(sharpest_move);
//        
//        // calculate the response
//        this->send_command("position fen " + pos.fen());
//        this->send_command("go depth " +  std::to_string(depth));
//        this->read(output, "bestmove", -1);
//        
//        // super janky, poi famo un parser come si deve eh
//        auto best_response_str = output[output.size()-1];
//        auto best_response = best_response_str.substr(9, 4);
//        
//        line.push_back(Utils::long_to_alg(pos, best_response));
//        std::cout << i << ". " << sharpest_move << " " << Utils::long_to_alg(pos, best_response) << " ";
//        pos.do_move(Utils::long_alg_to_move(pos, best_response));
//    }
//    std::cout << '\n';
//    
//    return line;
//}


