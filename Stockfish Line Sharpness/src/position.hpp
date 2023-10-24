//
//  position.hpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 24/10/2023.
//

#ifndef position_hpp
#define position_hpp

#include <stdio.h>

#include "mini_stock/bitboard.h"
#include "mini_stock/position.h"
#include "mini_stock/movegen.h"

class Position : public Stockfish::Position {
public:
    Position();
    Position(const std::string &FEN);
    
    inline Stockfish::MoveList<Stockfish::LEGAL> GetMoves() const { return Stockfish::MoveList<Stockfish::LEGAL>(*this); }
    
    Position& Set(const std::string &FEN);
    Position& Advance(const std::vector<Stockfish::Move> &moves);
private:
    Stockfish::StateInfo si_;
};


#endif /* position_hpp */
