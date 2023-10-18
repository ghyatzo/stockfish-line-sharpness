//
//  notation_translation.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 15/10/2023.
//

//#include "notation_translation.hpp"
#include "../src/utils.hpp"
#include "../src/mini_stock/position.h"
#include "../src/mini_stock/movegen.h"

using namespace Stockfish;

// Provide a custom position that offers all kinds of moves, either on black side or white side.
// Then list all those moves in all the formats.
// Make sure you check the move you write down is actually legal, and it's translated correctly between
// long and normal algebraic notation.
namespace Test {
    namespace White {
        static const std::string FEN {"1r2k2r/1pPp1p2/1b3qpn/4pP1p/pPB1P1bn/3P1NP1/P2BQ1NP/R3K2R w KQk - 0 1"};
        namespace LongAlg {
            static const std::vector<std::string> Legal {
                "e1a1", // castling
                "a2a3", "d2e3", "f3g5", // normal moves
                "f5g6", "d2h6", "g2h4", // captures
                "c4f7", // check
                "c7c8r", "c7c8b", // promotion to a rook or bishop
                "c7b8r" // promotion with capture (and check) into a rook
            };
            static const std::vector<std::string> Illegal {
                "e1h1", "h2h4", "e2h5", "a1b3" // illegal moves
            };
        }
        namespace Alg {
            static const std::vector<std::string> Legal {
                "O-O-O", // castling
                "a3", "Be3", "Ng5", // normal moves
                "fxg6", "Bxh6", "Ngxh4", // captures
                "Bxf7", // check
                "c8=R+", "c8=B", // promotions
                "cxb8=R+"
            };
            static const std::vector<std::string> Illegal {
                "O-O", "h4", "Qh5", "Rb3" // illegal moves
            };
        }
        namespace Moves {
            static const std::vector<Move> Legal {
                make<CASTLING>(Utils::coord_to_sq("e1"), Utils::coord_to_sq("a1")),
                make_move(Utils::coord_to_sq("a2"), Utils::coord_to_sq("a3")),
                make_move(Utils::coord_to_sq("d2"), Utils::coord_to_sq("e3")),
                make_move(Utils::coord_to_sq("f3"), Utils::coord_to_sq("g5")),
                make_move(Utils::coord_to_sq("f5"), Utils::coord_to_sq("g6")),
                make_move(Utils::coord_to_sq("d2"), Utils::coord_to_sq("h6")),
                make_move(Utils::coord_to_sq("g2"), Utils::coord_to_sq("h4")),
                make_move(Utils::coord_to_sq("c4"), Utils::coord_to_sq("f7")),
                make<PROMOTION>(Utils::coord_to_sq("c7"), Utils::coord_to_sq("c8"), ROOK),
                make<PROMOTION>(Utils::coord_to_sq("c7"), Utils::coord_to_sq("c8"), BISHOP),
                make<PROMOTION>(Utils::coord_to_sq("c7"), Utils::coord_to_sq("b8"), ROOK),
            };
            static const std::vector<Move> Illegal {
                make<CASTLING>(Utils::coord_to_sq("e1"), Utils::coord_to_sq("h1")),
                make_move(Utils::coord_to_sq("h2"), Utils::coord_to_sq("h4")),
                make_move(Utils::coord_to_sq("e2"), Utils::coord_to_sq("h5")),
                make_move(Utils::coord_to_sq("a1"), Utils::coord_to_sq("b3")),
            };
        }
        
    };
    namespace Black {
        static const std::string FEN {"1r2k2r/1pPp1p2/1b3qpn/4pP1p/pPB1P1bn/3P1NP1/P2BQ1NP/R3K2R b KQk b3 0 1"};
        namespace LongAlg {
            static const std::vector<std::string> Legal {
                "e8h8", // castling
                "d7d5", "b6d4", "h6g8", //normal moves
                "g6f5", "g4f3", "h4f5", // captures
                "b6f2", //check
                "a4b3" // enpassant
            };
            static const std::vector<std::string> Illegal {
                "a8d5", "f6d7", "e5e3", "e8a8" // illegal
            };
        }
        namespace Alg {
            static const std::vector<std::string> Legal {
                "O-O", // castling
                "d5", "Bd4", "Ng8", //normal moves
                "gxf5", "Bxf3", "N4xf5", // captures
                "Bf2+", //check,
                "axb3",
            };
            static const std::vector<std::string> Illegal {
                "Rd5", "Qd7", "e3", "O-O-O" // illegal
            };
        }
        namespace Moves {
            static const std::vector<Move> Legal {
                make<CASTLING>(Utils::coord_to_sq("e8"), Utils::coord_to_sq("h8")),
                make_move(Utils::coord_to_sq("d7"), Utils::coord_to_sq("d5")),
                make_move(Utils::coord_to_sq("b6"), Utils::coord_to_sq("d4")),
                make_move(Utils::coord_to_sq("h6"), Utils::coord_to_sq("g8")),
                make_move(Utils::coord_to_sq("g6"), Utils::coord_to_sq("f5")),
                make_move(Utils::coord_to_sq("g4"), Utils::coord_to_sq("f3")),
                make_move(Utils::coord_to_sq("h4"), Utils::coord_to_sq("f5")),
                make_move(Utils::coord_to_sq("b6"), Utils::coord_to_sq("f2")),
                make<EN_PASSANT>(Utils::coord_to_sq("a4"), Utils::coord_to_sq("b3")),
            };
            static const std::vector<Move> Illegal {
                make_move(Utils::coord_to_sq("a8"), Utils::coord_to_sq("d5")),
                make_move(Utils::coord_to_sq("f6"), Utils::coord_to_sq("d7")),
                make_move(Utils::coord_to_sq("e5"), Utils::coord_to_sq("e3")),
                make<CASTLING>(Utils::coord_to_sq("e8"), Utils::coord_to_sq("a8")),
            };
            
        }
    };
    
    bool MovesLongAlg(Position& pos, std::vector<Move> moves, std::vector<std::string> la_moves)
    {
        assert(la_moves.size() == moves.size());
        for (int i = 0; i < la_moves.size(); i++) {
            auto las = la_moves[i];
            auto m = moves[i];
            std::cout << "\tstr: " << las;
            auto mtola = Utils::to_long_alg(m);
            std::cout << "  translated: " << mtola;
            auto latom = Utils::long_alg_to_move(pos, mtola);
            std::cout << "  circleback: " << Utils::to_long_alg(latom) << std::endl;
            if (mtola != las || latom != m) return false;
        }
        return true;
    }
    
    bool MovesAlg(Position& pos, std::vector<Move> moves, std::vector<std::string> alg_moves)
    {
        assert(alg_moves.size() == moves.size());
        for (int i = 0; i < alg_moves.size(); i++) {
            auto alg = alg_moves[i];
            auto m = moves[i];
            std::cout << "\tstr: " << alg;
            auto mtoa = Utils::to_alg(pos, m);
            std::cout << "  translated: " << mtoa;
            auto atom = Utils::alg_to_move(pos, mtoa);
            std::cout << "  circleback: " << Utils::to_alg(pos, atom) << std::endl;
            if (mtoa != alg || atom != m) return false;
        }
        return true;
    }
    
    bool AlgLong(Position& pos, std::vector<std::string> alg_moves, std::vector<std::string> la_moves)
    {
        assert(la_moves.size() == alg_moves.size());
        for (int i = 0; i < alg_moves.size(); i++) {
            auto algs = alg_moves[i];
            auto lalgs = la_moves[i];
            std::cout << "\talg: " << algs << " long_alg: " << lalgs << '\n';
            auto latoa = Utils::long_to_alg(pos, lalgs);
            std::cout << "\t\tlong->alg: " << latoa << '\n';
            auto atola = Utils::alg_to_long(pos, algs);
            std::cout << "\t\talg->long: " << atola << '\n';
            if (algs != latoa || lalgs != atola) return false;
        }
        return true;
    }
    
    bool MovesAreLegal(Position& pos, std::vector<Move> legal, std::vector<Move> illegal)
    {
        for (const auto& ms : legal) {
            bool pseudo_legal = pos.pseudo_legal(ms);
            bool legal = pos.legal(ms);
            std::cout << "\tlegal move: " << Utils::to_long_alg(ms) << " legal: " << (pseudo_legal && legal) << std::endl;
            if (!(pseudo_legal && legal)) return false;
        }
        for (const auto& ms : illegal) {
            bool pseudo_legal = pos.pseudo_legal(ms);
            bool legal = pseudo_legal ? pos.legal(ms) : false;
            std::cout << "\tillegal move: " << Utils::to_long_alg(ms) << " legal: " << (pseudo_legal && legal) << std::endl;
            if (pseudo_legal && legal) return false;;
        }
        return true;
    }
}

int test_translations()
{
    Bitboards::init();
    Position::init();
    
    auto pos = Position();
    auto si = StateInfo();
    
    pos.set(Test::White::FEN, false, &si, NULL);
    
    std::cout << "Testing position for White: " << std::endl;
    std::cout << "Translating between Moves and Long Algebraic: \n";
    if (!Test::MovesLongAlg(pos, Test::White::Moves::Legal, Test::White::LongAlg::Legal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    std::cout << "Translating between Moves and Algebraic: \n";
    if (!Test::MovesAlg(pos, Test::White::Moves::Legal, Test::White::Alg::Legal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    std::cout << "Translating between Algebraic and Long Algebraic: \n";
    if (!Test::AlgLong(pos, Test::White::Alg::Legal, Test::White::LongAlg::Legal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    std::cout << "Moves are Legal: \n";
    if (!Test::MovesAreLegal(pos, Test::White::Moves::Legal, Test::White::Moves::Illegal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    pos.set(Test::Black::FEN, false, &si, NULL);
    
    std::cout << "Testing position for Black: " << std::endl;
    std::cout << "Translating between Moves and Long Algebraic: \n";
    if (!Test::MovesLongAlg(pos, Test::Black::Moves::Legal, Test::Black::LongAlg::Legal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    std::cout << "Translating between Moves and Algebraic: \n";
    if (!Test::MovesAlg(pos, Test::Black::Moves::Legal, Test::Black::Alg::Legal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    std::cout << "Translating between Algebraic and Long Algebraic: \n";
    if (!Test::AlgLong(pos, Test::Black::Alg::Legal, Test::Black::LongAlg::Legal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    std::cout << "Moves are Legal: \n";
    if (!Test::MovesAreLegal(pos, Test::Black::Moves::Legal, Test::Black::Moves::Illegal)) {
        std::cout << "Failed" << std::endl; exit(1);
    } std::cout << "Passed" << std::endl;
    
    return 0;
}
