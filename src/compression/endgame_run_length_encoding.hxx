#ifndef _ENDGAME_RUN_LENGTH_ENCODING_
#define _ENDGAME_RUN_LENGTH_ENCODING_

#include "../typedefs.hxx"

// Used for win/loss/draw information only!

// RunLengthEncoded_Endgame
class RLE_Endgame {
public:
  RLE_Endgame() : data(0), convert_table(0) {}
  ~RLE_Endgame() { if (data) { delete data; delete convert_table; } }

  void init(const int8_t *table, int size, int method = 2, int dont_care_value = 128);
  
private:
  void init_impl1(const int8_t *table, int size, int dont_care_value = 128);
  void init_impl2(const int8_t *table, int size, int dont_care_value = 128);

  uint8_t *data;
  int8_t *convert_table;
};

#endif
