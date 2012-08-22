#include "board_tables.hxx"

const bool IS_KNIGHT_OR_KING[13+2] =
{false,
 false, true, false, false, false, true,
 false, true, false, false, false, true,
 false, false};
const bool IS_PAWN_OR_KING[13+2] = {false,
				  true, false, false, false, false, true,
				  true, false, false, false, false, true,
                                  true, true};
const bool IS_KING[13+2] = {false,
			  false, false, false, false, false, true,
			  false, false, false, false, false, true,
                          false, false};
const bool IS_SHORT_DISTANCE_PIECE[13+2] =
{false,
 true, true, false, false, false, true,
 true, true, false, false, false, true,
 true, true};
const bool IS_LONG_DISTANCE_PIECE[13+2] =
{false,
 false, false, true, true, true, false,
 false, false, true, true, true, false,
 false, false};

const int direction_dc[8] = {1,0,1,-1,-1,0,-1,1};
const int direction_dr[8] = {0,1,1,1,0,-1,-1,-1};
const int DIRECTIONS[9] = {EAST, NORTH, NORTH_EAST, NORTH_WEST,
				  WEST, SOUTH, SOUTH_WEST, SOUTH_EAST, 0};
const string SDIRECTION_NAME[8] = {"east", "north", "north east", "north west",
					  "west", "south", "south west", "south east"};
const string UDIRECTION_NAME[4] = {"east or west", "north or south",
					  "north east or south west", "north west or south east"};

const bool BORDER_COLUMN[8] = {1,0,0,0,0,0,0,1};

const unsigned char BORDER_DISTANCE[64] =
  {0,0,0,0,0,0,0,0,
   0,1,1,1,1,1,1,0,
   0,1,2,2,2,2,1,0,
   0,1,2,3,3,2,1,0,
   0,1,2,3,3,2,1,0,
   0,1,2,2,2,2,1,0,
   0,1,1,1,1,1,1,0,
   0,0,0,0,0,0,0,0};
