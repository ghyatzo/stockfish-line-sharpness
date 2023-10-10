//
//  main.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 30/09/2023.
//

#include <iostream>
#include <ranges>
#include "utils.hpp"
#include "fen.hpp"
#include "stock_wrapper.hpp"

void print_usage()
{
    std::cout << "Usage is: line_sharpness -e <engine path> [-d <depth>] [-f '<FEN string>'] [-l] [-a] [<moves>...] \n";
    std::cout << "\t -h prints this message" << '\n';
    std::cout << "\t -e <path> the path must be absolute" << '\n';
    std::cout << "\t -d <int> default = 15" << '\n';
    std::cout << "\t -f '<FEN string>' default = starpos (use quotes)" << '\n';
    std::cout << "\t -l eval line flag" << '\n';
    std::cout << "\t -a short algebraic flag" << '\n';
    std::cout << "\t <moves>... set of moves relative to the position (use long algebraic notation)" << '\n';
    std::cout << "If the -l flag is not set, only the end position given by <FEN> + <moves> is analysed." << '\n';
    std::cout << "If the -l flag is passed, the whole line is analysed. There must be at least 1 move." << '\n';
    std::cout << "If not the -L flag does nothing. (I suggest you evaluate lines on a low depth, lest you like to watch paint dry)" << '\n';
    exit(0);
}

int main(int argc, const char * argv[])
{
    std::string FEN { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
    std::string stockfish_path {};
    std::vector<std::string> starting_moves {};
    bool line_flag {false};
    bool short_alg {false};
    int depth = 15;
    
    // We want to know what percentage of moves ends up in a overall worse position.
    // we consider a blunder losing 300+ cp, a mistake losing 120-300 cp and inaccuracies 50-120 cp.
    // depending on the ELO, you might consider changing these values.
    // for now lets just put everything under one "bad move umbrella", all the blunders and all the mistakes.
    // inaccuracies are considered neutral, the rest is good moves.
    double bad_move_delta_threshold = 3; // blunders
    double neutral_move_delta_threshold = 1.2; // mistakes (sono scarso dio caro).
    double good_move_delta_threshold = 0.5; // inaccuracy
    double it_not_matter_threshold = 10;
    
    int ch;
    while ((ch = getopt(argc, const_cast<char * const *>(argv), "hlad:e:f:")) != -1) {
        switch (ch) {
            case 'd': depth = std::stoi(optarg); break;
            case 'e': stockfish_path = optarg; break;
            case 'f': FEN = optarg; break;
            case 'a': short_alg = true; break;
            case 'l': line_flag = true; break;
            case 'h': print_usage();
            case '?': print_usage();
            default: print_usage();
        }
    }
    
    for (int i = optind; i < argc; i++) {
        starting_moves.push_back(argv[i]);
    }
    if (starting_moves.empty()) line_flag = false;
    
    if (int r = checkFEN(FEN.c_str()) < 0) {
        std::cout << "Please enter a valid FEN string: Bad FEN Err: " << r << std::endl;
        exit(-1);
    }
    
    auto stock = Stock(stockfish_path);
    stock.Start();
    
    stock.SetNewPosition(FEN);
    
    if (line_flag)
    {
        std::vector<double> sharpnesses {};
        sharpnesses.reserve(starting_moves.size()+1);
        double ratio = stock.PositionSharpness(depth, neutral_move_delta_threshold,
                                               good_move_delta_threshold, it_not_matter_threshold);
        std::cout << "starting pos - sharpness: " << ratio << '\n';
        sharpnesses.emplace(sharpnesses.end(), ratio);
        for (const auto& ms : starting_moves) {
            Move mm = short_alg ? Utils::alg_to_move(stock.pos, ms)
            : Utils::long_alg_to_move(stock.pos, ms);
    
            ratio = stock.PositionSharpness(depth, neutral_move_delta_threshold,
                                                   good_move_delta_threshold, it_not_matter_threshold);
            
            std::cout << "input move: " << ms
            << " alg: " << Utils::to_alg(stock.pos, mm)
            << " long_alg: " << Utils::to_long_alg(mm)
            << " - sharpness: " << ratio << '\n';
            
            sharpnesses.emplace(sharpnesses.end(), ratio);
            
            stock.pos.do_move(mm);
        }
        double average_s = 0;
        for (auto a : sharpnesses) {
            average_s += a;
        }
        average_s /= sharpnesses.size()+1;
        std::cout << " Average line sharpness: " << average_s << '\n';
        
    }
    else
    {
        for (auto ms : starting_moves) {
            Move mm = short_alg ? Utils::alg_to_move(stock.pos, ms)
            : Utils::long_alg_to_move(stock.pos, ms);
            

            stock.pos.do_move(mm);
        }
        
        std::cout << stock.pos << std::endl;
        
        // set eventual options with: setoption name <id> [value <xy>]
        
        double base_eval = stock.EvalPosition(depth, -1);
        std::cout << "Eval: " <<base_eval << " (depth: " << depth << ")" << std::endl;
        
        // Get legal moves.
        auto moves = stock.GetMoves();
        std::cout << "In This position there are " << moves.size() << " possible moves. " << (stock.pos.side_to_move() ? "Black" : "White") << " to move" << std::endl;
        
        auto evals = stock.EvalMoves(moves, depth, -1);
        
        auto [ok_moves, bad_moves] =
        stock.ComputeSharpness(evals, base_eval, neutral_move_delta_threshold,
                               good_move_delta_threshold, it_not_matter_threshold);
        
        // print the ratio
        std::cout << "Bad moves: " << bad_moves << " (loss >= " << neutral_move_delta_threshold << ")\n";
        std::cout << "Ok moves: " << ok_moves << " (loss < " << good_move_delta_threshold << ")\n";
        std::cout << "not optimal, but not terrible: " << moves.size() - ok_moves - bad_moves << "\n";
        std::cout << "Sharpness ratio of: " << "(bad/(ok+bad)) " << bad_moves <<
        "/(" << ok_moves << "+" << bad_moves << ") = " << bad_moves/(ok_moves+bad_moves) << std::endl;
        
        // show the "better than inaccuracies" moves.
        auto mi = moves.begin();
        auto ei = evals.begin();
        for (int i = 0; i < moves.size(); i++) {
            auto m = *mi;
            auto e = *ei;
            std::cout << "[" << e << "]\t";
            std::cout << Utils::to_alg(stock.pos, m) << " (" << Utils::to_long_alg(m) << ")" << '\n';
            
            //        auto delta = std::abs(base_eval - e);
            //        if (delta <= good_move_delta_threshold){
            //            std::cout << "[" << e << "]\t";
            //            std::cout << Utils::to_alg(stock.pos, m) << " (" << Utils::to_long_alg(m) << ")" << '\n';
            //        }
            
            if (ei < evals.end() && mi < moves.end()) { ei++; mi++; }
        }
    }
    
    
    
//    std::string input {};
//    while (true) {
//        std::getline(std::cin, input);
//        if (input.empty()) continue;
//        if (input == "exit") break;
//        // valid fen, run the program again
//        if (checkFEN(input.c_str()) == 0) {
//            std::cout << "sadly this feature is still not implemented" << std::endl;
//        } else { // send commands to stockfish directly
//            stock.send_command(input);
//            stock.Read("", 1500);
//            Utils::print_output(stock.output);
//        }
//    }
}
