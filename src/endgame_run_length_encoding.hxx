#ifndef _ENDGAME_RUN_LENGTH_ENCODING_
#define _ENDGAME_RUN_LENGTH_ENCODING_

#include "typedefs.hxx"

// Used for win/loss/draw information only!

// RunLengthEncoded_Endgame
class RLE_Endgame {
public:
  RLE_Endgame() : data(0), convert_table(0) {}
  ~RLE_Endgame() { if (data) { delete data; delete convert_table; } }

  void init(const char *table, int size, int method = 2, int dont_care_value = 128);
  
private:
  void init_impl1(const char *table, int size, int dont_care_value = 128);
  void init_impl2(const char *table, int size, int dont_care_value = 128);

  uchar *data;
  char *convert_table;
};

#endif
