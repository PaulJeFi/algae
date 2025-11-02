# Algae chess engine

<img src="./logo/logo.png" width="300" height="300">
Algae is a UCI chess engine written in C++.

- [Board representation](https://www.chessprogramming.org/Board_Representation): [Bitboards](https://www.chessprogramming.org/Bitboards)
	- [Magic Bitboard / PEXT Bitboards](https://www.chessprogramming.org/Magic_Bitboards)
- [Evaluation](https://www.chessprogramming.org/Evaluation)
    - [PSQT](https://www.chessprogramming.org/Piece-Square_Tables) eval
    - [material balance](https://www.chessprogramming.org/Material)
    - [tapered eval](https://www.chessprogramming.org/Tapered_Eval)
    - [Mop-up evaluation](https://www.chessprogramming.org/Mop-up_Evaluation)
    - [Trapped bishops](https://www.chessprogramming.org/Trapped_Pieces)
    - [Minor pieces development](https://www.chessprogramming.org/Development)
    - [Center control](https://www.chessprogramming.org/Center_Control) : [Pawn center](https://www.chessprogramming.org/Pawn_Center)
    - [Unadvanced central pawns](https://www.chessprogramming.org/Development#Eval_Considerations)
    - [Rook on open and semi-open files](https://www.chessprogramming.org/Rook_on_Open_File)
    - [Bishop Pair](https://www.chessprogramming.org/Bishop_Pair)
    - [Isolated Pawn](https://www.chessprogramming.org/Isolated_Pawn)
    - [Doubled Pawn](https://www.chessprogramming.org/Doubled_Pawn)
- [Search](https://www.chessprogramming.org/Search)
    - [PVS](https://www.chessprogramming.org/Principal_Variation_Search)
    - [TT](https://www.chessprogramming.org/Transposition_Table)
        - [Zobrist Hashing](https://www.chessprogramming.org/Zobrist_Hashing)
    - [Null Move Pruning](https://www.chessprogramming.org/Null_Move_Pruning)
    - [Razoring](https://www.chessprogramming.org/Razoring#LimitedRazoring)
    - [LMR](https://www.chessprogramming.org/Late_Move_Reductions) (_to test_)
    - [iterative deepening](https://www.chessprogramming.org/Iterative_Deepening)
    - [mate distance pruning](https://www.chessprogramming.org/Mate_Distance_Pruning)
    - [check extensions](https://www.chessprogramming.org/Check_Extensions) (check-evaders for the moment)
    - [Quiescence Search](https://www.chessprogramming.org/Quiescence_Search)
        - [delta pruning](https://www.chessprogramming.org/Delta_Pruning)
    - [Move Orderning](https://www.chessprogramming.org/Move_Ordering)
        - [MVV LVA](https://www.chessprogramming.org/MVV-LVA)
        - [History Heuristic](https://www.chessprogramming.org/History_Heuristic)
        - [Killer Moves Heuristic](https://www.chessprogramming.org/Killer_Heuristic)
        - [TT score](https://www.chessprogramming.org/Transposition_Table)

## A note on BMI2

[Sliding pieces](https://www.chessprogramming.org/Sliding_Pieces) [move generation](https://www.chessprogramming.org/Move_Generation) uses [magic bitboards](https://www.chessprogramming.org/Magic_Bitboards). If your computer supports [BMI2](https://www.chessprogramming.org/BMI2), Algae uses the [PEXT bitboards](https://www.chessprogramming.org/BMI2#PEXTBitboards) optimization to make the engine faster. You can force using the BMI2 optimization (if supported by your computer) with ```make bmi2```.
## Thanks

- Tord Romstad and Maksim Korzh for the explaination and code of Magic Bitboards generation and use.
- Disservin for his [Smallbrain engine](https://github.com/Disservin/Smallbrain), heavily inspiring the make/unmake move method and the time management.
- Slender for his [Rice engine](https://github.com/rafid-dev/rice), heavily inspiring the UCI function.