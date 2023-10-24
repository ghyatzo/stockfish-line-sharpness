//
//  position.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 24/10/2023.
//

#include "position.hpp"
#include "utils.hpp"
#include "fen.hpp"

Position::Position() : Stockfish::Position(), si_()
{
    Set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
Position::Position(const std::string &fen) : Stockfish::Position(), si_()
{
    Stockfish::Bitboards::init();
    Stockfish::Position::init();
    Set(fen);
}

Position& Position::Set(const std::string &fen)
{
    if (checkFEN(fen.c_str()) < 0) throw std::runtime_error("Please enter a valid FEN string");
    set(fen, false, &si_);
    
    return *this;
}

Position& Position::Advance(const std::vector<Stockfish::Move> &moves)
{
    for (const auto mm : moves) { do_move(mm); }
    return *this;
}
