//
//  parsing_test.cpp
//  Tests
//
//  Created by Camillo Schenone on 18/10/2023.
//

#include <stdio.h>
#include <vector>
#include <string>
#include <ranges>

#include "../src/utils.hpp"
#include "notation_translation.hpp"

static const std::vector<std::string> output_example = {
    "info string NNUE evaluation using nn-0000000000a0.nnue",
    "info depth 1 seldepth 1 multipv 1 score mate 1 wdl 1000 0 0 nodes 87 nps 29000 hashfull 0 tbhits 0 time 3 pv f3f7",
    "info depth 5 seldepth 4 multipv 2 score cp 205 wdl 997 3 0 nodes 453 nps 151000 hashfull 0 tbhits 0 time 3 pv c4f7 e8e7 f7b3",
    "bestmove f3f7",
};

int test_parsing()
{
    {
        std::string best_move = Utils::parse_best_move(output_example);
        std::cout << "[Test][bestmove parsing] \t best move: " << best_move << " - ";
        if (best_move != "f3f7") {
            std::cout << "Failed" << std::endl; std::abort();
        } std::cout << "Passed" << std::endl;
    }
    {
        std::vector<std::string> bad_output {output_example.begin(), output_example.end()-1};
        std::string best_move = Utils::parse_best_move(bad_output);
        std::cout << "[Test][bestmove parsing] \t best move: " << best_move << " - ";
        if (best_move != MOVE_NONE_STR) {
            std::cout << "Failed" << std::endl; std::abort();
        } std::cout << "Passed" << std::endl;
    }
    {
        std::vector<std::string> score_line_cp { output_example[2] };
        std::string score = Utils::parse_score(score_line_cp);
        auto cp = Utils::centipawns(Color::WHITE, score_line_cp);
        auto value = Utils::cp_to_value(cp * 100);
        auto [W, D, L] = Utils::parse_wdl(score_line_cp);
        std::cout << "[Test][score parsing] \t score: " << score
        << " cp: " << cp
        << " value: " << value
        << " WDL: " << W << "/" << D << "/" << L << " - ";
        if (score != "205" || W != 997 || D != 3 || L != 0) {
            std::cout << "Failed" << std::endl; std::abort();
        } std::cout << "Passed" << std::endl;
    }
    {
        std::vector<std::string> score_line_mate {output_example[1]};
        auto score = Utils::parse_score(score_line_mate);
        auto cp = Utils::centipawns(Color::WHITE, score_line_mate);
        auto value = Utils::cp_to_value(cp * 100);
        auto [W, D, L] = Utils::parse_wdl(score_line_mate);
        std::cout << "[Test][score parsing] \t score: " << score
        << " cp: " << cp
        << " value: " << value
        << " WDL: " << W << "/" << D << "/" << L << " - ";
        if (score != "1m" || W != 1000 || D != 0 || L != 0) {
            std::cout << "Failed" << std::endl; std::abort();
        } std::cout << "Passed" << std::endl;
    }



    return 0;
}

int main()
{
    test_translations();
    test_parsing();
}
