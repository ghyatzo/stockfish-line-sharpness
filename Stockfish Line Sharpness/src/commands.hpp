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

std::vector<double> LineSharpness(Engine&, const std::vector<Stockfish::Move>&, Position&);

double PositionSharpness(Engine&, Position&);

#endif /* commands_hpp */
