#ifndef _GAME_PHASE_
#define _GAME_PHASE_

#define SUM_MATERIAL (2*(8*1 + 2*3 + 2*3 + 2*5 + 9)) // 78
#define MAX_MATERIAL (SUM_MATERIAL + 2*8*(9-1)) // 78+128 = 206

extern const int MATERIAL[13];
extern const int MATERIAL2[13];

#define OPENING_GAME 0
#define MID_GAME 1
#define END_GAME 2

extern const int GAME_PHASE_PLY[128][3]; // each triple must sum to 256
extern const int GAME_PHASE_MATERIAL[MAX_MATERIAL+1][3]; // each triple must sum to 256

#endif
