//
//  fen.cpp
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 07/10/2023.
//

#include "fen.hpp"
#include <iostream>

int checkFEN(const char* FEN)
{
    char c, *p;
    // just do a format check. not a full blown legality check.
    int kings, file, pos;
    kings = file = pos = 0;
    // keep track of pieces number and position to do some checks afterwards.
    int w_pieces_c[5] {};
    int b_pieces_c[5] {};
    int Q[10], q[10], R[10], r[10], N[10], n[10], BL[5], BD[5], bl[5], bd[5];
    for (int i = 0; i < 10; i++) Q[i] = q[i] = R[i] = r[i] = N[i] = n[i] = -1;
    for (int i = 0; i < 5; i++) BL[i] = BD[i] = bl[i] = bd[i] = -1;
    int w_pawns[8] {};
    int b_pawns[8] {};
    bool white, K_castl, Q_castl, k_castl, q_castl;
    white = K_castl = Q_castl = k_castl = q_castl = false;
    
    p = const_cast<char*>(FEN);
    for (int row = 7; row >= 0; row--) {
        file = 0;
        do {
            c = *p++;
            if (c >= '1' && c <= '8') {
                // implicitly cast to int, and get the difference from the ASCII value for '0'
                file += c - '0';
                if (file > 8) return -2; //empty space overflow // file consistency error
            } else {
                white = true;
                pos = row*8 + file;
                // if we encounter a lower case letter, "cast it to uppercase" but consider it black.
                if (c >= 'a') { c += 'A' - 'a'; white = false; }
                switch (c)
                {
                    case 'K':
                        if (kings > 1) return -3; // piece consistency error
                        // king not in starting position, no castling rights.
                        if ((row == 0 || row == 7) && (file == 4)) {
                            if (white) { K_castl = Q_castl = true; }
                            else { k_castl = q_castl = true; }
                        }
                        kings++;
                        break;
                    case 'P':
                        // no pawns in the first/last row
                        if (row == 0 || row == 7) return -4; // illegal position error
                        white ? w_pawns[file]++ : b_pawns[file]++;
                        break;
                    case 'R': {
                        // castling rights
                        if (row == 0) {
                            if (file == 0) if (white) Q_castl = true;
                            if (file == 7) if (white) K_castl = true;
                        }
                        if (row == 7) {
                            if (file == 0) if (!white) q_castl = true;
                            if (file == 7) if (!white) k_castl = true;
                        }
                        // too many (c)ooks
                        if (w_pieces_c[ROOK] > 9 || b_pieces_c[ROOK] > 9) return -3;
                        white ? R[w_pieces_c[ROOK]++] = pos : r[b_pieces_c[ROOK]++] = pos;
                        break;
                    case 'B':
                        if (row % 2) { // ood rows means odd files are light.
                            if (file % 2) {
                                if (white && w_pieces_c[LBISHOP] > 4) return -3;
                                if (!white && b_pieces_c[LBISHOP] > 4) return -3;
                                white ? BL[w_pieces_c[LBISHOP]++] = pos : bl[b_pieces_c[LBISHOP]++] = pos;
                            } else {
                                if (white && w_pieces_c[DBISHOP] > 4) return -3;
                                if (!white && b_pieces_c[DBISHOP] > 4) return -3;
                                white ? BD[w_pieces_c[DBISHOP]++] = pos : bd[b_pieces_c[DBISHOP]++] = pos;
                            }
                        } else {
                            // even rows means even files are light.
                            if (file % 2) { // odd files
                                if (white && w_pieces_c[DBISHOP] > 4) return -3;
                                if (!white && b_pieces_c[DBISHOP] > 4) return -3;
                                white ? BD[w_pieces_c[DBISHOP]++] = pos : bd[b_pieces_c[DBISHOP]++] = pos;
                            } else {
                                if (white && w_pieces_c[LBISHOP] > 4) return -3;
                                if (!white && b_pieces_c[LBISHOP] > 4) return -3;
                                white ? BL[w_pieces_c[LBISHOP]++] = pos : bl[b_pieces_c[LBISHOP]++] = pos;
                            }
                        }
                        break;
                    case 'Q':
                        if (w_pieces_c[QUEEN] > 9 || b_pieces_c[QUEEN] > 9) return -3;
                        white ? Q[w_pieces_c[QUEEN]++] = pos : q[b_pieces_c[QUEEN]++] = pos;
                        break;
                    case 'N':
                        if (w_pieces_c[KNIGHT] > 9 || b_pieces_c[KNIGHT] > 9) return -3;
                        white ? N[w_pieces_c[KNIGHT]++] = pos : n[b_pieces_c[KNIGHT]++] = pos;
                        break;
                    default: return -5;
                    }
                }
                file++;
            }
        } while (file < 8);
        
        if(file == 8)
        {
            c = *p++;
            if(row > 0 && c != '/') return -5; /* bad format */
            if(row==0  && c != ' ') return -5;
        }
    }
    
    int w_maj_pieces_c { sum(w_pieces_c, 5) };
    int b_maj_pieces_c { sum(b_pieces_c, 5) };
    int w_tot_pieces = w_maj_pieces_c + sum(w_pawns, 8);
    int b_tot_pieces = b_maj_pieces_c + sum(b_pawns, 8);
//    int w_tot_extra, b_tot_extra;
//    w_tot_extra = b_tot_extra = 0;
//    for(int i = 0; i < 5; i ++) {
//        int w_extra, b_extra;
//        if (i == ROOK || i == KNIGHT) {
//            w_extra = extra_pieces(w_maj_pieces_c, 2);
//            b_extra = extra_pieces(b_maj_pieces_c, 2);
//        } else {
//            w_extra = extra_pieces(w_maj_pieces_c, 1);
//            b_extra = extra_pieces(b_maj_pieces_c, 1);
//        }
//        // - if one side has extra pieces, the opposing side must have less than 16 pieces.
//        if (w_extra && (b_tot_pieces == 16)) return -3;
//        if (b_extra && (w_tot_pieces == 16)) return -3;
//        
//        w_tot_extra += w_extra;
//        b_tot_extra += b_extra;
//    }
//    //  - more promoted pieces than missing pawns
//    if (w_tot_extra > 8 - sum(w_pawns, 8)) return -3;
//    if (b_tot_extra > 8 - sum(b_pawns, 8)) return -3;
    
    // no more than 6 pawn in a column
    for (auto p: w_pawns) if (p > 6) return -4;
    for (auto p: b_pawns) if (p > 6) return -4;
    
    /*
     enough enemy pieces have been exchanged to allow for that formation.
     from column C to F:        for columns B and G:    for columns A and H:
     2 pawns = 1 exchange        2=1                     2=1
     3 pawns = 2 exchanges       3=2                     3=3
     4 pawns = 4 exchanges       4=4                     4=6
     5 pawns = 6 exchanges       5=7                     5=10
     6 pawns = 9 echanges        6=11                    6=15
     */
    int AH[7] {0, 0, 1, 3, 6, 10, 15};
    int CF[7] {0, 0, 1, 2, 4, 6, 9};
    int BG[7] {0, 0, 1, 2, 4, 7, 11};
    for (int i = 0; i < 8; i++) {
        
        if (i == 0 || i == 8) { // column A & H
            if ( b_tot_pieces + AH[w_pawns[i]] > 15 ) return -4;
            if ( w_tot_pieces + AH[b_pawns[i]] > 15 ) return -4;
            continue;
        }
        if (i == 1 || i == 7) { //column B & G
            if ( b_tot_pieces + BG[w_pawns[i]] > 15 ) return -4;
            if ( w_tot_pieces + BG[b_pawns[i]] > 15 ) return -4;
            continue;
        }
        
        // column C to F
        if ( b_tot_pieces + CF[w_pawns[i]] > 15 ) return -4;
        if ( w_tot_pieces + CF[b_pawns[i]] > 15 ) return -4;
        
    }
    
    /*
     still need to check for:
     
     - if there are multiple pawns in a column, there can't be (both) pawns
     in the adjacent files and one row lower.
     - bishops in the first and last ranks that are blocked by enemy pawns in their starting positions.
     or bishops in the first and last ranks, at the corners, blocked by a friendly pawn.
     */
    
    
    // check for the presence of the white or black turn
    c= *p++;
    if (c != 'w' && c != 'b') return -5;
    c = *p++;
    if (c != ' ') return -5;
    
    // castling rights
    
    bool end = false;
    while ((c = *p++)) {
        switch (c) {
            case 'K': if(!K_castl) return -6; break;
            case 'Q': if(!Q_castl) return -6; break;
            case 'k': if(!k_castl) return -6; break;
            case 'q': if(!k_castl) return -6; break;
            case '-': end = true; p++; break;
            case ' ': end = true; break;
            default: return -5;
        }
        if (end) break;
    }
    
    // en passant square:
    c = *p++;
    if ((c >= 'a' && c <= 'h')) {
        if (*p == '3' || *p == '6') p++;
        else return -7;
    } else if (c == '-');
    else return -5;
    
    while ((c = *p++)) {
        if (c >= '0' && c <= '9') continue;
        else if (c == ' ') continue;
        else return -5;
    }
    
    return 0;
}
