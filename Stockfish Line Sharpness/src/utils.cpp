//
//  utils.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 09/10/2023.
//

#include <sstream>
#include <numeric>
#include <ranges>
#include "utils.hpp"

namespace Utils {
    using namespace Stockfish;
    
    // Piece and square translation
    char pt_to_char(PieceType pt) { return "  NBRQK"[pt]; }
    PieceType char_to_pt(char c)
    {
        switch (c) {
            case ' ': return PieceType::PAWN;
            case 'N': return PieceType::KNIGHT;
            case 'R': return PieceType::ROOK;
            case 'B': return PieceType::BISHOP;
            case 'Q': return PieceType::QUEEN;
            case 'K': return PieceType::KING;
            default: return PieceType::NO_PIECE_TYPE;
        }
    }
    
    Square coord_to_sq(std::string coord)
    {
        assert(coord.size() == 2);
        return make_square(File(coord[0] - 'a'), Rank((coord[1] - '0') - 1));
    }
    
    std::string to_coord(Square s) 
    {
        return std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
    }
    
    // Move notation translation
    std::string to_alg(Position &pos, Move m)
    {
        auto sq { to_sq(m) };
        auto piece { type_of(pos.moved_piece(m)) };
        auto num_N = pos.count<KNIGHT>(pos.side_to_move());
        auto num_R = pos.count<ROOK>(pos.side_to_move());
        auto num_Q = pos.count<QUEEN>(pos.side_to_move());
        auto num_B= pos.count<BISHOP>(pos.side_to_move());
        std::string disc = "";
        std::string move {};
        
        // castling logic
        if (type_of(m) == MoveType::CASTLING) {
            if (pos.side_to_move() == WHITE) {
                return to_coord(sq) == "h1" ? "O-O" : "O-O-O";
            } else {
                return to_coord(sq) == "h8" ? "O-O" : "O-O-O";
            }
        }
        // discriminant
        if ( (piece != PAWN ) && (num_N > 1 || num_R > 1 || num_B > 2 || num_Q > 1) ) {
            std::vector<Move> candidates {};
            for (const auto om : MoveList<LEGAL>(pos)) {
                if (to_sq(m) == to_sq(om) && pos.moved_piece(m) == pos.moved_piece(om)) candidates.push_back(om);
            }
            if (candidates.size() == 2) { // bishops and rooks can have at most two candidates.
                for (const auto cm : candidates) {
                    // compare the file and rank of all candidates moves with the right move.
                    
                    // if there is another piece that can reach the same end square on a different row
                    // use the row as discriminant
                    if (file_of(from_sq(cm)) != file_of(from_sq(m))) {
                        disc += std::string(1, 'a' + file_of(from_sq(m))); break;
                    }
                    if (rank_of(from_sq(cm)) != rank_of(from_sq(m))) {
                        disc += std::string(1, '1' + rank_of(from_sq(m))); break;
                    }
                }
            }
            if (candidates.size() > 2) { // 3 or more knights or queens
                disc += to_coord(from_sq(m));
            }
        }
        // promotion
        if (type_of(m) == PROMOTION) {
            PieceType pt {promotion_type(m)};
            if (pos.capture(m))
                move += std::string(1, char('a' + file_of(from_sq(m)))) + "x" + to_coord(sq);
            else move += to_coord(sq);
            move += "=" + std::string(1, pt_to_char(pt));
            if (pos.gives_check(m)) move += "+";
            return move;
        }
        // capture
        if (pos.capture(m)) {
            if (piece == PAWN)
                return std::string(1, char('a' + file_of(from_sq(m)))) + "x" + to_coord(sq);
            
            return std::string(1, pt_to_char(piece)) + disc + "x" + to_coord(sq);
        }
        
        move = to_coord(sq);
        if (pos.gives_check(m)) move += "+";
        
        return (piece == PAWN) ? move : pt_to_char(piece) + disc + move;
    }
    
    std::string to_long_alg(Move m)
    {
        if (m == MOVE_NONE)
            return MOVE_NONE_STR;
        if (m == MOVE_NULL)
            return MOVE_NULL_STR;
        
        auto from { from_sq(m) };
        auto to   { to_sq(m) };
        
        if (type_of(m) == CASTLING)
            to = make_square(to > from ? FILE_H : FILE_A, rank_of(from));
        
        std::string move {to_coord(from) + to_coord(to)};
        
        if (type_of(m) == PROMOTION) {
            move += " pnbrqk"[promotion_type(m)];
        }
        
        return move;
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
    
    std::string alg_to_long(Position &pos, std::string alg)
    {
        PieceType piece;
        Square from;
        Square to;
        Bitboard positions;
        std::string disc {};
        std::vector<std::string> candidate_moves {};
        // remove the check marker, we don't care in long notation.
        if (alg.ends_with('+')) alg = alg.substr(0, alg.size()-1);
        
        // castling
        if (alg == "O-O" || alg == "O-O-O") {
            piece = KING;
            from = pos.square<KING>(pos.side_to_move());
            to = make_square((alg == "O-O") ? FILE_H : FILE_A, rank_of(from));
            is_ok(to); is_ok(from);
            return to_coord(from) + to_coord(to);
        }
        
        if (alg.size() == 2) { // pawn move
            to = coord_to_sq(alg);
            piece = PAWN;
            if (rank_of(to) != RANK_4 && rank_of(to) != RANK_5) {
                Rank r = pos.side_to_move() == WHITE ? Rank(alg[1] - '1' - 1) : Rank(alg[1] - '1' + 1);
                from = make_square(file_of(to), r);
                is_ok(to); is_ok(from);
                return to_coord(from) + to_coord(to);
            }
        } else if (alg.size() == 3) { // Piece Move
            to = coord_to_sq(alg.substr(1, 2));
            piece = char_to_pt(alg[0]);
        } else if (alg.size() == 4) { // Capture, pawn promotion, disambig move
            if (size_t pos = alg.find('x'); pos != std::string::npos) { // it's a capture.
                assert(pos == 1); // the x is the second character, otherwise we have a malformed string.
                to = coord_to_sq(alg.substr(2)); // last two.
                piece = (alg[0] >= 'a' && alg[0] <= 'h') ? PAWN : char_to_pt(alg[0]);
            } else if (size_t pos = alg.find('='); pos != std::string::npos) { // promotion
                assert(pos == 2); // the = is the third character, otherwise we ha a malformed string.
                to = coord_to_sq(alg.substr(0, 2));
                from = make_square(File(alg[0] - 'a'), Rank(alg[1] - '1' - 1));
                is_ok(to); is_ok(from);
                return to_coord(from) + to_coord(to) + std::string(1, tolower(alg[3]));
            } else { // we are disambiguating a piece move
                assert((alg[1] >= 'a' && alg[1] <= 'h') || (alg[1] >= '1' && alg[1] <= '8'));
                to = coord_to_sq(alg.substr(2));
                piece = char_to_pt(alg[0]);
                disc = alg[1];
            }
        } else if (alg.size() == 5) { // disambig capture, double disambig move
            if (size_t pos = alg.find('x'); pos != std::string::npos) {
                assert(pos == 2); // x in the 3rd position
                assert((alg[1] >= 'a' && alg[1] <= 'h') || (alg[1] >= '1' && alg[1] <= '8'));
                to = coord_to_sq(alg.substr(3));
                piece = char_to_pt(alg[0]);
                disc = alg[1];
            } else { // double d move
                to = coord_to_sq(alg.substr(3));
                piece = char_to_pt(alg[0]);
                disc = alg.substr(1, 2);
            }
        } else if (alg.size() == 6) { // promotion with capture, double disambig capture
            if (size_t pos = alg.find('='); pos != std::string::npos) { // we have a promotion with capture.
                assert(pos == 4);
                pos = alg.find('x');
                assert(pos == 1);
                to = coord_to_sq(alg.substr(2, 2));
                from = make_square(File(alg[0] - 'a'), RANK_7);
                is_ok(to); is_ok(from);
                return to_coord(from) + to_coord(to) + std::string(1, tolower(alg[5]));
            } else {
                assert(alg.find('x') == 3);
                to = coord_to_sq(alg.substr(4,2));
                piece = char_to_pt(alg[0]);
                disc = alg.substr(1, 2);
            }
        } else {
            return MOVE_NONE_STR;
        }
        
        assert(piece != NO_PIECE_TYPE);
        is_ok(to);
        positions = pos.pieces(pos.side_to_move(), piece);
        while (popcount(positions)) {
            from = lsb(positions);
            candidate_moves.push_back(to_coord(from) + to_coord(to));
            positions ^= from;
        }
        for (const auto& s : candidate_moves) {
            Move m = long_alg_to_move(pos, s);
            if (m == MOVE_NONE) continue;
            if (disc.empty()) return s;
            else if (disc.size() == 1) {
                // return the move that starts from the same rank of file of the discriminant
                if (   (std::string(1, 'a' + file_of(from_sq(m))) == disc)
                    || (std::string(1, '1' + rank_of(from_sq(m))) == disc) ) return s;
            } else { // double disc
                // return the move that has the same from square as the disc
                if (to_coord(from_sq(m)) == disc) return s;
            }
        }
        // if nothing was found return a null move.
        return MOVE_NONE_STR;
    }
    
    std::string long_to_alg(Position &pos, std::string str)
    {
        Move m = long_alg_to_move(pos, str);
        return to_alg(pos, m);
    }

    Move alg_to_move(Position &pos, std::string alg) {
        std::string long_alg = alg_to_long(pos, alg);
        return long_alg_to_move(pos, long_alg);
    }

    
    // info parsing
    void print_output(const std::vector<std::string> & output, std::string prefix)
    {
        for (auto s : output) {
            std::cout << prefix << s << std::endl;
        }
    }
    
    std::string parse_best_move(const std::vector<std::string> & output) 
    {
        std::string token;
        std::istringstream is;
        for (const auto& s : std::views::reverse(output)) { // bestmove is usually the last line.
            is.str(s);
            while (is >> std::skipws >> token) {
                if (token == "bestmove") {
                    is >> std::skipws >> token;
                    return token;
                }
            }
        }
        return MOVE_NONE_STR;
    }
    
    // TODO: support multiPV, gotta select manually for now (maybe another function?)
    std::string parse_score(const std::string & info_line)
    {
        std::istringstream is {info_line};
        std::string token;
        while (is >> std::skipws >> token) {
            if (token == "cp") {
                is >> std::skipws >> token;
                return token;
            }
            if (token == "mate") {
                is >> std::skipws >> token;
                //append an "m" to indicate mates
                return token+"m";
            }
        }
        throw std::runtime_error("failed to parse cp/mate score: bad format.");
    }
    
    std::tuple<int, int, int> parse_wdl(const std::string & info_line)
    {
        std::istringstream is {info_line};
        std::string token;
        int W, D, L;
        while (is >> std::skipws >> token) {
            if (token == "wdl") {
                is >> W >> D >> L;
                return std::make_tuple(W, D, L);
            }
        }
        throw std::runtime_error("failed to parse WDL score: bad format. (make sure to enable the UCI_showWDL option).");
    }
    
    Value cp_to_value(int cp) { return Value(cp * NormalizeToPawnValue / 100); };
    int to_cp(Value v) { return 100 * v / NormalizeToPawnValue; }
    
    double centipawns(Color col, const std::string &output)
    {
        // normalise centipawns and mate values to a singol decimal value.
        std::string score { parse_score(output) };
        Value v {};
        if (score.ends_with('m')) {
            int mate_ply = std::stoi(score.substr(0, score.size()-1));
            v = mate_ply < 0 ? mated_in(mate_ply) : mate_in(mate_ply);
        } else {
            v = cp_to_value(std::stoi(score));
        }
        
        double cp = to_cp(v);
        return col == Color::BLACK ? -1*cp/100.0 : cp/100.0;
        
    }
    void sort_evals_perm(std::vector<int> &perm, const std::vector<double> &evals)
    {
        std::sort(perm.begin(), perm.end(), [&](int a, int b){
            return evals[a] > evals[b];
        });
    }
    
    std::vector<int> sort_evals_perm(const std::vector<double> &evals)
    {
        // work on a permutation vector.
        std::vector<int> perm(evals.size());
        std::iota(perm.begin(), perm.end(), 0);
        
        std::sort(perm.begin(), perm.end(), [&](int a, int b){
            return evals[a] > evals[b];
        });

        return perm;
    }
} // namespace Utils
