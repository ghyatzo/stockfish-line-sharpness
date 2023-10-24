//
//  commands.hpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 20/10/2023.
//

#ifndef commands_hpp
#define commands_hpp

#include <stdio.h>

#include "stock_wrapper.hpp"

std::vector<double> whole_line_sharpness(Stock &stock, const std::vector<Move> &starting_moves, int depth);

double end_position_sharpness(Stock &stock, const std::vector<Move> &starting_moves, int depth);

#endif /* commands_hpp */
