//
//  main.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 30/09/2023.
//

#include <iostream>
#include <ranges>
#include <span>

#include "stock_wrapper.hpp"
#include "utils.hpp"
#include "sharpness.hpp"
#include "commands.hpp"

class Arguments {
public:
    static void s_print_usage()
    {
        std::cout << "Usage is: line_sharpness -e <engine path> [-d <depth>] [-f '<FEN string>'] [-l] [-a] [-G <length>] [<moves>...] \n";
        std::cout << "\t -h prints this message" << '\n';
        std::cout << "\t -e <path> the path must be absolute" << '\n';
        std::cout << "\t -d <int> default = 15" << '\n';
        std::cout << "\t -f '<FEN string>' default = startpos (use quotes)" << '\n';
        std::cout << "\t -l eval whole line flag" << '\n';
        std::cout << "\t -a short algebraic flag" << '\n';
        std::cout << "\t -G <int> generate sharp line flag" << '\n';
        std::cout << "\t <moves>... set of moves relative to the position (use long algebraic notation)" << '\n';
        
        std::cout << "- Compute the sharpness of the position given by <FEN> and after making the specified <moves>." << '\n';
        std::cout << "- Pass the -l flag to evaluate the sharpness at every step when applying the <moves>. (I suggest you evaluate lines on a low depth, lest you like to watch paint dry)" << '\n';
        std::cout << "- Pass the -G <length> to generate the sharpest line of the specified length, starting from the given position (plus eventual <moves>)." << '\n';
        std::cout << "- Pass the -I flag to enable interactive mode with the specified engine in UCI mode." << '\n';
        std::cout << "" << '\n';
        std::exit(0);
    }
    
    Arguments(int argc, char * const argv[])
        :args_{argv, static_cast<size_t>(argc)}
    {
        int ch;
        while ((ch = getopt(argc, argv, "hlaIG:d:e:f:")) != -1) {
            switch (ch) {
                case 'd': depth_            = std::stoi(optarg); break;
                case 'e': engine_path_      = optarg; break;
                case 'f': fen_string_       = optarg; break;
                case 'a': short_alg_        = true; break;
                case 'l': whole_line_       = true; break;
                case 'I': interactive_      = true; break;
                case 'G': {
                    generate_line_          = true;
                    generate_line_length_   = std::stoi(optarg);
                    break;
                }
                case 'h': s_print_usage();
                case '?': s_print_usage();
                default: s_print_usage();
            }
        }

        if (engine_path_.empty()) s_print_usage();
        
        if (whole_line_ && generate_line_) {
            std::cout << "The -l and -G flags are mutually exclusive, choose one." << '\n';
            s_print_usage();
        }
        
        if (optind == argc) whole_line_ = false;
        
        for (int i {optind}; i < argc; i++) {
            moves_.push_back(args_[i]);
        }
    }
    Arguments(const Arguments &other) = delete;
    Arguments& operator=(const Arguments &other) = delete;
    
    std::string engine_path() {return engine_path_;}
    std::string init_fen() {return fen_string_;}
    bool short_alg_notation() {return short_alg_;}
    
    bool interactive() {return interactive_;}
    bool whole_line() {return whole_line_;}
    bool generate_line() {return generate_line_;}
    size_t gen_line_length() {return generate_line_length_;}
    
    int depth() {return depth_;}
    size_t size() {return args_.size();}
    
    std::vector<std::string>& moves() {return moves_;}
    
private:
    std::string engine_path_ {};
    std::string fen_string_ { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
    bool whole_line_ {false};
    bool interactive_ {false};
    bool generate_line_ {false};
    size_t generate_line_length_ {};
    bool short_alg_ {false};
    int depth_ {15};
    
    std::span<char * const> args_;
    std::vector<std::string> moves_ {};
};

void print_moves(Engine &engine, Position& pos)
{
    auto moves = pos.GetMoves();
    auto evals = engine.EvalMoves(moves, pos);
    auto base_eval = engine.Eval(pos);
    auto sorted_perm = Utils::sort_evals_perm(evals, pos.side_to_move());
    std::cout << "Using the expected game score metric: " << std::endl;

    for (const auto i : sorted_perm) {
        auto relative_eval = base_eval - evals[i];
        relative_eval = pos.side_to_move() == Stockfish::WHITE ? -relative_eval : relative_eval;
        std::cout << " " << evals[i] << "\t[" << relative_eval << "]\t";
        std::cout << Utils::to_alg(pos, moves[i]) << "\t(" << Utils::to_long_alg(moves[i]) << ")" << '\n';
        if (relative_eval < -0.9 ) {
            std::cout << " ... here be exceptionally bad moves " << std::endl;
            break;
        }
    }
}

double average_sharpness(const std::vector<double> &sharpnesses, Stockfish::Color col, Stockfish::Color starting_col)
{
    auto it = sharpnesses.cbegin();
    int num_moves = (static_cast<int>(sharpnesses.size()) + 1) / 2;
    if (col != starting_col) {
        it++;
        num_moves = static_cast<int>(sharpnesses.size()) / 2;
    }
    double accumulate {};
    while (it < sharpnesses.cend()) {
        accumulate += *it;
        it += 2;
    }
    
    return accumulate / static_cast<double>(num_moves);
}

Stockfish::Color starting_color(int n_ply, Stockfish::Color ending_color)
{
    // operator~ toggles the color.
    if (n_ply % 2 == 0) { return ~ending_color; }
    else { return ending_color; }
}

int main(int argc, char * const argv[])
{
    auto args = Arguments(argc, argv);
    auto engine = Engine(args.engine_path());
    auto starting_pos = Position(args.init_fen());
    
    auto moves = Utils::translate_moves(starting_pos, args.moves(), args.short_alg_notation());
    engine.Start();
    
    if (args.whole_line()) 
    {
        std::cout << "Line analysis:" << std::endl;
        std::cout << "Loaded Starting Position: \n" << starting_pos << std::endl;

        PositionSharpness(engine, starting_pos);
        // just compute the lines, then analyse.
        auto sharpness = LineSharpness(engine, moves, starting_pos);
        
        std::cout << "Sharpness by Move:" << std::endl;
        // sharpness also has the sharpness for the starting position. while moves do not.
        //TODO: make this sturdier
        for (int i {1}; i < sharpness.size(); i++) {
            std::cout << "( " << Utils::to_alg(starting_pos, moves[i-1]) << " )\t" << sharpness[i] << std::endl;
            starting_pos.DoMove(moves[i-1]);
        }
        
        auto start_col = starting_color((int)sharpness.size(), starting_pos.side_to_move());
        auto white_sharp_avg = average_sharpness(sharpness, Stockfish::WHITE, start_col);
        auto black_sharp_avg = average_sharpness(sharpness, Stockfish::BLACK, start_col);
         
        std::cout << "White has an average line sharpness of: " << white_sharp_avg << std::endl;
        std::cout << "Black has an average line sharpness of: " << black_sharp_avg << std::endl;
    }
    else if (args.generate_line())
    {
        std::cout << "Sharp Line Generation" << std::endl;
        std::cout << "stepping through moves..." << std::endl;
        starting_pos.Advance(moves);
        std::cout << starting_pos << std::endl;

        auto line = Sharpness::GenerateLine(args.gen_line_length(), starting_pos, engine);
        
        Utils::print_output(line);
    }
    else {
        std::cout << "Sharpness analysis:" << std::endl;
        std::cout << "stepping through moves..." << std::endl;
        starting_pos.Advance(moves);
        std::cout << starting_pos << std::endl;
        PositionSharpness(engine, starting_pos);
        print_moves(engine, starting_pos);
    }
    
//    if (args.interactive()) {
//        using namespace std::chrono_literals;
//        
//        std::string input {};
//        while (true) {
//            std::getline(std::cin, input);
//            if (input.empty()) continue;
//            if (input == "exit") break;
//            // send commands to stockfish directly
//            engine.send_command(input);
//            engine.Read("", 1500ms);
//            Utils::print_output(engine.output);
//        }
//    }
}
