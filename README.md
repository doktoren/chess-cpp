# chess-cpp

TLDR: Don't waste your time by looking here.

Source code (C++98) for the program made in my master thesis: Generation and compression of endgame tables in chess with fast random access using OBDDs.

More information is available [here](https://jespertk.dk/master_thesis/) (hasn't been updated for a long time).

# Bugs

When I wrote this program I made use of several optimizations that we're based on how the compiler I was using happened to behave.
In some cases this was a deliberate (but bad) choice. In most cases I didn't know better.

These are some of the mistakes that I've been hit by:
* I assumed that a `char` was an unsigned 8 bit integer.
* I assumed that I could write to one union member and read the corresponding bits from another union member

A few times I've tried to rectify the errors and make the program work again.
But this is hard work with 35k lines of poorly documented code :-/

Currently large parts of it is working (in a dockerized build environment) but there are still significant problems.

Known bugs
* Some short versions of commands causes crash (e.g. "ctfi ..." instead of "construct from table index ...")
* With retrograde construction many (all?) invalid positions are stored as draw (-125) instead of illegal (-128). This reduces how much the tables can be compressed as OBDDs.
* The 5 piece KPPKP endgame cannot be constructed as it contains a position with a depth to mate of 127 (`8/8/8/8/1p2P3/4P3/1k6/3K4 w - - 0 1`). This cannot fit in the `uint8_t` as values -124 and below are reserved.
* When compiled against XBoard an error is logged when quitting the program (`free(): double free detected in tcache 2`)
* Retrograde construction of KBNKP fails for a position with a pawn at an invalid square: 
```
Error: Retrograde constr.: Player from index = 9217
(it is 1 to move)
Num checks = 0
    a b c d e f g h
  +-----------------+    |
8 |                 | 8  | 50b
7 |                 | 7  |
6 |                 | 6  | White has lost castling
5 |                 | 5  | Black has lost castling
4 |                 | 4  |
3 |                 | 3  | moves played since progress = 0
2 | N               | 2  |
1 | k p K B         | 1  |
  +-----------------+    |
    a b c d e f g h
FEN: loadfen 8/8/8/8/8/8/N7/kpKB4 b - - 0 50
```


# What seems to work

* The endgames tables that are built seems to be correct. At least they are correct for some longest mates for 5 piece endgames ([test.com](src/test.com))
* The chess engine can be used by xboard (tested with version 4.9.1). As command use `make run_xb` using the root of this repository as folder. This assumes you have docker installed. Maybe you should run `make run_xb` first to prebuild the image.
