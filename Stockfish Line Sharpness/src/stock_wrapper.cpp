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

void Engine::SetOption(const std::string & optname, const std::string & optvalue)
{
    if (optname == "UCI_showWDL" ) this->opts_.showWDL = optvalue == "true" ? true : false;
    if (optname == "Threads" ) this->opts_.threads = std::stoi(optvalue);
    if (optname == "MultiPV" ) this->opts_.multiPV = std::stoi(optvalue);
        
    this->send_command("setoption " + optname + " value " + optvalue);
}

bool Engine::Read(const std::string &expected, std::chrono::milliseconds timeout)
{
    return this->read(output, expected, int(timeout.count()));
}

void Engine::NewGame()
{
    this->send_command("ucinewgame");
    this->send_command("isready");
    
    // wait for stockfish to be ready
    this->read(output, "readyok");
}

std::string Engine::GetBestMove(Position& pos)
{
    this->send_command("position fen " + pos.fen());
    this->send_command("go depth " + std::to_string(depth_));
    Read("bestmove");
    
    return Utils::parse_best_move(output);
}

double Engine::Eval(Position& pos)
{
    this->send_command("position fen " + pos.fen());
    this->send_command("go depth " +  std::to_string(depth_));
    Read("bestmove");
    
    return Utils::centipawns(pos.side_to_move(), output[output.size()-2]);
}

double Engine::Eval(Stockfish::Move m, Position& pos)
{
    // evaluates the move without changing pos.
    this->send_command("position fen " + pos.fen() + " moves " + Utils::to_long_alg(m));
    this->send_command("go depth " + std::to_string(depth_));
    Read("bestmove");
    
    // measuring the first depths takes less than a ms, so we're safe.
    return Utils::centipawns(~pos.side_to_move(), output[output.size()-2]);
}

void Engine::Eval(std::vector<double> &evals, const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos)
{
    evals.resize(moves.size());
    for (int count {}; const auto m: moves) {
        evals.emplace(evals.begin()+count, Eval(m, pos));
        PROGRESS_BAR(count);
        count++;
    }
}

std::vector<double> Engine::Eval(const Stockfish::MoveList<Stockfish::LEGAL> &moves, Position& pos)
{
    std::vector<double> evals;
    evals.reserve(moves.size());
    for (int count {}; const auto m: moves) {
        evals.emplace_back(Eval(m, pos));
        PROGRESS_BAR(count);
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


