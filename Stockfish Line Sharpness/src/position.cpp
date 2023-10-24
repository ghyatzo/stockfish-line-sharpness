//
//  position.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 24/10/2023.
//

#include "position.hpp"
#include "utils.hpp"
#include "fen.hpp"

Position::Position() : Stockfish::Position()
{
    Stockfish::Bitboards::init();
    Stockfish::Position::init();
    
    StateInfoList_ = std::make_unique<std::deque<Stockfish::StateInfo>>();
    Set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
Position::Position(const std::string &fen) : Stockfish::Position()
{
    Stockfish::Bitboards::init();
    Stockfish::Position::init();
    
    StateInfoList_ = std::make_unique<std::deque<Stockfish::StateInfo>>();
    Set(fen);
//    set(fen, false, &si);
}

Position& Position::Set(const std::string &fen)
{
    if (checkFEN(fen.c_str()) < 0) throw std::runtime_error("Please enter a valid FEN string");
    // deletes all previous states
    StateInfoList_->erase(StateInfoList_->cbegin(), StateInfoList_->cend());
    StateInfoList_->push_back(Stockfish::StateInfo());
    set(fen, false, &StateInfoList_->back());
    
    return *this;
}

void Position::DoMove(Stockfish::Move m) {
    StateInfoList_->push_back(Stockfish::StateInfo());
    do_move(m, StateInfoList_->back());
}

Position& Position::Advance(const std::vector<Stockfish::Move> &moves)
{
    for (const auto mm : moves) { DoMove(mm); }
    return *this;
}
