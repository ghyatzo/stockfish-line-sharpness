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

void advancePosition(Stockfish::Position &pos, const std::vector<std::string> &moves, bool long_alg)
{
    for (auto ms : moves) {
        Move mm = long_alg ? Utils::long_alg_to_move(pos, ms)
        : Utils::alg_to_move(pos, ms);

        pos.do_move(mm);
    }
}

int main(int argc, const char * argv[])
{
    std::string FEN { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
    std::string stockfish_path {};
    std::vector<std::string> starting_moves {};
    bool line_flag {false};
    bool generate_line_flag {false};
    int generate_line_length {};
    bool short_alg {false};
    int depth = 15;
    
    // We want to know what percentage of moves ends up in a overall worse position.
    // we consider a blunder losing 300+ cp, a mistake losing 120-300 cp and inaccuracies 50-120 cp.
    // depending on the ELO, you might consider changing these values.
    // for now lets just put everything under one "bad move umbrella", all the blunders and all the mistakes.
    // inaccuracies are considered neutral, the rest is good moves.
    constexpr double blunder_threshold = 3; // blunders
    constexpr double mistake_threshold = 1.1; // mistakes (sono scarso dio caro).
    constexpr double inaccuracy_threshold = 0.5; // inaccuracy
    
    int ch;
    while ((ch = getopt(argc, const_cast<char * const *>(argv), "hlaG:d:e:f:")) != -1) {
        switch (ch) {
            case 'd': depth = std::stoi(optarg); break;
            case 'e': stockfish_path = optarg; break;
            case 'f': FEN = optarg; break;
            case 'a': short_alg = true; break;
            case 'l': line_flag = true; break;
            case 'G': {
                generate_line_flag = true;
                generate_line_length = std::stoi(optarg);
                break;
            }
            case 'h': print_usage();
            case '?': print_usage();
            default: print_usage();
        }
    }
    
    if (line_flag && generate_line_flag) {
        std::cout << "The -l and -G flags are mutually exclusive, choose one." << '\n';
        print_usage();
    }
    
    if (stockfish_path.empty())
        print_usage();
    
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
        std::cout << "Loaded Starting Position: \n" << stock.pos << std::endl;
        std::vector<double> sharpnesses {};
        sharpnesses.reserve(starting_moves.size()+1);
        
        auto [ratio, blunders] = stock.PositionSharpness(depth, blunder_threshold, mistake_threshold,
                                               inaccuracy_threshold);
        sharpnesses.emplace(sharpnesses.end(), ratio);
        std::cout << "Eval: " << stock.EvalPosition(depth, -1) << " (depth: " << depth << ")" << std::endl;
        std::cout << "starting pos - sharpness: " << ratio << '\n';
        
        for (const auto& ms : starting_moves) {
            Move mm = short_alg ? Utils::alg_to_move(stock.pos, ms)
                                : Utils::long_alg_to_move(stock.pos, ms);
            
            stock.pos.do_move(mm);
            auto num_moves = stock.GetMoves().size();
            auto [ratio, blunders] = stock.PositionSharpness(depth, blunder_threshold, mistake_threshold,
                                                   inaccuracy_threshold);
            
            std::cout << " ms :\t blunders: " << blunders << " sharpness: " << ratio << '\n';
            // ratio = bad_moves/(good_moves + bad_moves) and num_moves - blunders = good_moves + bad_moves.
            auto bad_moves = ratio*(num_moves-blunders);
            auto good_moves = num_moves-blunders-bad_moves;
            std::cout << "\t bad_moves: " << bad_moves << " good moves: " << good_moves << '\n';
            sharpnesses.emplace(sharpnesses.end(), ratio);
            
        }
        // TODO, split white and black averages
        double w_average_s = 0;
        double b_average_s = 0;
        for (int i = 0; i < sharpnesses.size(); i++) {
            w_average_s += sharpnesses[i];
        }
        w_average_s /= sharpnesses.size()+1;
        std::cout << " Average line sharpness: " << w_average_s << '\n';
        
    }
    else if (generate_line_flag)
    {
        advancePosition(stock.pos, starting_moves, !short_alg);
        std::cout << stock.pos << std::endl;
        auto line = stock.GenerateSharpLine(generate_line_length, depth, blunder_threshold, mistake_threshold, inaccuracy_threshold);
    }
    else
    {
        advancePosition(stock.pos, starting_moves, !short_alg);
        std::cout << stock.pos << std::endl;
        
        auto [ratio, blunders] =
        stock.PositionSharpness(depth, blunder_threshold, mistake_threshold, inaccuracy_threshold);

        auto moves = stock.GetMoves();
        auto evals = stock.EvalMoves(moves, depth);
        auto num_moves = moves.size();
        auto bad_moves = ratio*(num_moves-blunders);
        auto ok_moves = num_moves-blunders-bad_moves;
        // print the ratio
        
        double base_eval = stock.EvalPosition(depth, -1);
        std::cout << "Eval: " <<base_eval << " (depth: " << depth << ")" << std::endl;
        std::cout << "In This position there are " << num_moves << " possible moves.\n"
                  << (stock.pos.side_to_move() ? "Black" : "White") << " to move" << std::endl;
        std::cout << "Bad moves: " << bad_moves << " (loss >= " << mistake_threshold << ")\n";
        std::cout << "Ok moves: " << ok_moves << " (loss < " << inaccuracy_threshold << ")\n";
        std::cout << "blunders: " << blunders << "\n";
        std::cout << "Sharpness ratio of: " << "(bad/(ok+bad)) " << bad_moves <<
        "/(" << ok_moves << "+" << bad_moves << ") = " << bad_moves/(ok_moves+bad_moves) << std::endl;
        
        std::cout << "Good Moves: " << '\n';
        // show the "better than inaccuracies" moves.
        auto mi = moves.begin();
        auto ei = evals.begin();
        for (int i = 0; i < moves.size(); i++) {
            auto m = *mi;
            auto e = *ei;
            
            std::cout << "[" << e << "]\t";
            std::cout << Utils::to_alg(stock.pos, m) << " (" << Utils::to_long_alg(m) << ")" << '\n';

//            auto delta = std::abs(base_eval - e);
//            if (delta <= good_move_delta_threshold){
//                std::cout << "[" << e << "]\t";
//                std::cout << Utils::to_alg(stock.pos, m) << " (" << Utils::to_long_alg(m) << ")" << '\n';
//            }
            
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
