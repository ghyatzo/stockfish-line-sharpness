//
//  utils.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 09/10/2023.
//

#include "utils.hpp"

namespace Utils {
    using namespace Stockfish;
    
    char pt_to_char(PieceType pt)
    {
        std::string piece_types {"  NBRQK"};
        return piece_types[pt];
    }
    
    PieceType char_to_pt(char c)
    {
        switch (c) {
            case ' ': return PieceType::PAWN;
            case 'n':
            case 'N': return PieceType::KNIGHT;
            case 'r':
            case 'R': return PieceType::ROOK;
            case 'b':
            case 'B': return PieceType::BISHOP;
            case 'q':
            case 'Q': return PieceType::QUEEN;
            case 'k':
            case 'K': return PieceType::KING;
            default: return PieceType::NO_PIECE_TYPE;
        }
    }
    
    Square pt_to_sq(Position &pos, PieceType pt)
    {
        switch (pt) {
            case PAWN: return pos.square<PAWN>(pos.side_to_move());
            case ROOK: return pos.square<ROOK>(pos.side_to_move());
            case KNIGHT: return pos.square<KNIGHT>(pos.side_to_move());
            case BISHOP: return pos.square<BISHOP>(pos.side_to_move());
            case QUEEN: return pos.square<QUEEN>(pos.side_to_move());
            case KING: return pos.square<KING>(pos.side_to_move());
            default: return SQ_NONE;
        }
    }
    
    Square coord_to_sq(std::string coord)
    {
        assert(coord.size() == 2);
        Rank r { (coord[1] - '0') - 1 };
        File f { (coord[0] - 'a') };
        return make_square(f, r);
    }
    
    std::string to_square(Square s) {
        return std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
    }
    
    std::string to_alg(Position &pos, Move m)
    {
        auto sq { to_sq(m) };
        auto piece { type_of(pos.moved_piece(m)) };
        // castling logic
        if (type_of(m) == MoveType::CASTLING) {
            auto sqsq = to_square(sq);
            if (pos.side_to_move() == WHITE) {
                return to_square(sq) == "h1" ? "O-O" : "O-O-O";
            } else {
                return to_square(sq) == "h8" ? "O-O" : "O-O-O";
            }
        }
        // capture
        if (pos.capture(m)) {
            return std::string(1, pt_to_char(piece)) + "x" + to_square(sq);
        }
        // promotion
        if (type_of(m) == PROMOTION) {
            PieceType pt {promotion_type(m)};
            return to_square(sq) + "=" + pt_to_char(pt);
        }
        
        return pt_to_char(piece) + to_square(sq);
    }
    
    std::string to_long_alg(Move m)
    {
        if (m == MOVE_NONE)
            return "(none)";
        if (m == MOVE_NULL)
            return "0000";
        
        auto from { from_sq(m) };
        auto to   { to_sq(m) };
        
        if (type_of(m) == CASTLING)
            to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));
        
        std::string move {to_square(from) + to_square(to)};
        
        if (type_of(m) == PROMOTION) {
            move += " pnbrqk"[promotion_type(m)];
        }
        
        return move;
    }
    
    std::string alg_to_long(Position &pos, std::string alg)
    {
        PieceType piece;
        Square from;
        Square to;
        Bitboard positions;
        char disc = ' ';
        std::vector<std::string> candidate_moves {};
        // castling
        if (alg == "O-O" || alg == "O-O-O") {
            piece = KING;
            from = pos.square<KING>(pos.side_to_move());
            to = make_square((alg == "O-O") ? FILE_G : FILE_C, rank_of(from));
            return to_square(from) + to_square(to);
        } else if (alg.find('=') != std::string::npos ) { // promotion
            assert(alg.size() == 4);
            to = coord_to_sq(alg.substr(0, 2));
            from = make_square(file_of(to), pos.side_to_move() == WHITE ? RANK_7 : RANK_2 );
            return to_square(from) + to_square(to) + std::string(1, tolower(alg[5]));
        } else {
            piece = (alg[0] >= 'a' && alg[0] <= 'h') ? PAWN : char_to_pt(alg[0]);
            if (alg.find('x') != std::string::npos) {
                // we have to consider a specification of ambiguous move Rab1/Rhb1
                disc = ((alg[1] >= 'a' && alg[1] <= 'h')
                        || (alg[1] >= '1' && alg[1] <= '8')) ? alg[1] : ' ';
                to = coord_to_sq(alg.substr(disc == ' ' ? 2 : 3));
            } else {
                if (alg.size() == 2) to = coord_to_sq(alg);
                else if (alg.size() == 3) to = coord_to_sq(alg.substr(1));
                else {
                    disc = alg[1];
                    to = coord_to_sq(alg.substr(2));
                }
            }
            positions = pos.pieces(piece);
            while (popcount(positions)) {
                from = lsb(positions);
                candidate_moves.push_back(to_square(from) + to_square(to));
                positions ^= from;
            }
            for (const auto& s : candidate_moves) {
                Move m = long_alg_to_move(pos, s);
                if (m == MOVE_NONE) continue;
                if (disc == ' ') return s; // only one legal move possible
                // if the candidate square matches the discriminant
                if (   (char('a' + file_of(from_sq(m))) == disc)
                    || (char('1' + rank_of(from_sq(m))) == disc) ) return s;
            }
        }
        
        return "(null)";
    }
    
    std::string long_to_alg(Position &pos, std::string str)
    {
        Move m = long_alg_to_move(pos, str);
        return to_alg(pos, m);
    }
    
    Move long_alg_to_move(Position &pos, std::string str)
    {
        if (str.length() == 5)
            str[4] = char(tolower(str[4])); // The promotion piece character must be lowercased
        
        for (const auto& m : MoveList<LEGAL>(pos)) {
//            std::cout << to_long_alg(m) << '\n';
            if (str == to_long_alg(m))
                return m;
        }
        
        return MOVE_NONE;
    }
    Move alg_to_move(Position &pos, std::string alg) {
        std::string long_alg = alg_to_long(pos, alg);
        return long_alg_to_move(pos, long_alg);
    }
    
    void print_output(std::vector<std::string> & output, std::string prefix)
    {
        for (auto s : output) {
            std::cout << prefix << s << std::endl;
        }
    }
    
    double extract_eval(std::string eval_str)
    {
        std::string res;
        for (auto i = eval_str.begin() + 16; i < eval_str.end(); i++) {
            if (*i == '+' || *i == '-') {
                while (*i != ' ') {
                    res += *i;
                    i++;
                }
                break;
            }
        }
        return std::stod(res);
    }
    
    //info depth 14 seldepth 19 multipv 1 score cp 289 wdl 1000 0 0 nodes 2529 nps 126450 hashfull 0 tbhits 0 time 20 pv f7g6 ...
    double centipawns(Color col, std::string &output)
    {
        std::string res {};
        size_t cp_pos = output.find("cp");
        size_t mate_pos = output.find("mate");
        size_t offset;
        if (mate_pos != std::string::npos) {
            offset = mate_pos + 5;
        } else if ( cp_pos != std::string::npos ) {
            offset = cp_pos + 3;
        } else {
            return 0;
        }
        
        for (auto i = output.begin() + offset; i < output.end(); i++) {
            if (*i == ' ') break;
            res += *i;
        }
        
        // cp are relative to the player. so a positive cp is good for the current player.
        // We want to give a position relative score, so the cp are *-1 for the black.
        
        // if we have a checkmate, signal it with 9999 for white or -9999 for black.
        double cp = std::stod(res);
        if (mate_pos != std::string::npos) {
            if (cp <= 5) col == Color::BLACK ? cp = -9999 : cp = 9999;
            else if (cp > 5 && cp <= 10) col == Color::BLACK ? cp = -5555 : cp = 5555;
            else col == Color::BLACK ? cp = -1111 : cp = 1111;
        } else cp = cp / 100;
        
        return col == Color::BLACK ? -1*cp : cp;
    }
} // namespace Stock
