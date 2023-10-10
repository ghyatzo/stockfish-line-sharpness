//
//  fen.hpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 07/10/2023.
//

#ifndef fen_hpp
#define fen_hpp

# define QUEEN 0
# define ROOK 1
# define KNIGHT 2
# define LBISHOP 3
# define DBISHOP 4

# define COLERR -2    // column consistency error
# define PIECERR -3   // pieces consistency error
# define ILLPOSERR -4 // illegal position error
# define FORMERR -5   // string badly formatted
# define CASTERR -6   // inconsistent castling
# define EPSQERR -7   // inconsistent en passant square

inline int sum(int* arr, int len) { int s = 0; for(int i = 0; i < len; i++) s += arr[i]; return s;}
inline int extra_pieces(int piece_count, int expected_number)
{
    int diff = piece_count - expected_number;
    return diff < 0 ? 0 : diff;
}
inline int to_col(const char c) { return c - 'A'; }
inline char to_file(const int f) { return (char)((int)'A' + f); }

int checkFEN(const char * FEN);
#endif /* fen_hpp */
