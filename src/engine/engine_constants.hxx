#ifndef _ENGINE_CONSTANTS_
#define _ENGINE_CONSTANTS_

// Nothing may evaluate to more/less than +/-WIN
#define INF 0x7FFFFFFF
#define WIN 0x003FFFFF

#define LOG_MAX_GAME_LENGTH 10
#define MAX_GAME_LENGTH 1<<LOG_MAX_GAME_LENGTH

#define MAX_POSSIBLE_MOVES 100 // Need not be a power of 2

#define MAX_SEARCH_DEPTH 64 // Need not be a power of 2
#define MAX_CALC_DEPTH MAX_SEARCH_DEPTH

#define MAX_MATE_DEPTH 512
#define ORACLE_WIN (WIN/2)
#define ORACLE_LOSS (-ORACLE_WIN)
#define GUARANTEED_WIN (WIN-MAX_MATE_DEPTH)
#define GUARANTEED_LOSS (-GUARANTEED_WIN)

#endif
