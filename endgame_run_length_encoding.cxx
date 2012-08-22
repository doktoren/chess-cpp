#include "endgame_run_length_encoding.hxx"

#include "run_length_encoding/bit_stream.hxx"
#include "run_length_encoding/huffman_codec.hxx"

#include <map>
#include <vector>


void RLE_Endgame::init(const char *table, int size, int method, int dont_care_value) {
  switch (method) {
  case 1:
    init_impl1(table, size, dont_care_value);
    break;
  case 2:
    init_impl2(table, size, dont_care_value);
    break;
  default:
    assert(0);
  }
}

void RLE_Endgame::init_impl1(const char *table, int size, int dont_care_value) {
  int _count[257];
  memset(_count, 0, sizeof(int)*257);
  int *count = &(_count[128]);
  for (int i=0; i<size; i++) ++count[table[i]];

  int mapped_to = dont_care_value;
  if (count[dont_care_value]) {
    // Map dont care values to most frequently occuring value
    int best = 128;
    for (int i=-128; i<128; i++)
      if (count[i] > count[best]  &&  i != dont_care_value) best = i;

    if (best == 128) {
      cerr << "Warning: No care value in table.\n";
    } else {
      count[best] += count[dont_care_value];
      count[dont_care_value] = 0;
      mapped_to = best;
    }
  }

  int num_different_values = 0;
  for (int i=-128; i<128; i++) {
    if (count[i]  &&  i!=dont_care_value) ++num_different_values;
  }
  convert_table = (char *)malloc(num_different_values);
  num_different_values = 0;
  int _remap[257];
  int *remap = &(_remap[128]);
  for (int i=-128; i<128; i++) {
    if (count[i]  &&  i!=dont_care_value) {
      //cout << "value = " << i << "\n";
      remap[i] = num_different_values;
      convert_table[num_different_values++] = i;
    }
  }
  remap[dont_care_value] = remap[mapped_to];

  cerr << "Num. diff. values = " << num_different_values << "\n";
  if (num_different_values > 3) {
    cerr << "RLE_Endgame (so far) only works with at most 3 different care values.\n";
    exit(1);
  }

  data = (uchar *)malloc(size);
  obstream<array_obstream> out(data);
  out.mark();

  const bool CONV[4][4] =
    {{0, 0, 1, 0},
     {0, 0, 1, 0},
     {0, 1, 0, 0},
     {0, 0, 0, 0}};

  int last = remap[(int)table[0]];
  int c = 1;
  out.writeByte(last);
  for (int i=1; i<size; i++) {
    if (remap[table[i]] == last) {
      ++c;
    } else {
      int next = remap[(int)table[i]];
      out.writeEliasNumber(c);
      out.writeBit(CONV[last][next]);
      last = next;
      c = 1;
    }
  }
  out.writeEliasNumber(c);

  cerr << "Size of run length encoded endgame = "
       << (out.num_bits_since_mark()+7)/8 << " bytes.\n";
}



void RLE_Endgame::init_impl2(const char *table, int size, int dont_care_value) {
  //test_bit_streams("test");

  int _count[257];
  memset(_count, 0, sizeof(int)*257);
  int *count = &(_count[128]);
  for (int i=0; i<size; i++) ++count[table[i]];

  int mapped_to = dont_care_value;
  if (count[dont_care_value]) {
    // Map dont care values to most frequently occuring value
    int best = 128;
    for (int i=-128; i<128; i++)
      if (count[i] > count[best]  &&  i != dont_care_value) best = i;

    if (best == 128) {
      cerr << "Warning: No care value in table.\n";
    } else {
      count[best] += count[dont_care_value];
      count[dont_care_value] = 0;
      mapped_to = best;
    }
  }

  int num_different_values = 0;
  for (int i=-128; i<128; i++)
    if (count[i]  &&  i!=dont_care_value) ++num_different_values;
  convert_table = (char *)malloc(num_different_values);
  num_different_values = 0;
  int _remap[256];
  int *remap = &(_remap[128]);
  for (int i=-128; i<128; i++)
    if (count[i]  &&  i!=dont_care_value) {
      //cerr << "value = " << i << "\n";
      remap[i] = num_different_values;
      convert_table[num_different_values++] = i;
    }
  remap[dont_care_value] = remap[mapped_to];

  cerr << "Num. diff. values = " << num_different_values << "\n";
  if (num_different_values > 3) {
    cerr << "RLE_Endgame (so far) only works with at most 3 different care values.\n";
    exit(1);
  }

  vector<vector<pair<uint, int> > > elem_counts;
  {
    elem_counts.resize(num_different_values);
    vector<map<uint, int> > block_sizes;
    block_sizes.resize(num_different_values);

    int last = remap[(int)table[0]];
    int c = 1;
    for (int i=1; i<size; i++) {
      if (remap[table[i]] == last) {
	++c;
      } else {
	++block_sizes[last][c];
	c = 1;
	last = remap[(int)table[i]];
      }
    }
    ++block_sizes[last][c];

    typedef map<uint, int>::iterator CI;
    for (int i=0; i<num_different_values; i++) {
      int index = 0;
      elem_counts[i].resize(block_sizes[i].size());
      for (CI ci=block_sizes[i].begin(); ci!=block_sizes[i].end(); ci++) {
	elem_counts[i][index++] = *ci;
      }
    }
  }

  data = (uchar *)malloc(size);
  
  {
    obstream<array_obstream> out(data);
    out.mark();

    vector<Huffman<uint, Elias_int_streamer> * > huff;
    huff.resize(num_different_values);
    for (int i=0; i<num_different_values; i++) {
      /*
	cerr << "Value " << i << ":\n";
	for (uint j=0; j<elem_counts[i].size(); j++)
	cerr << "(" << elem_counts[i][j].first << "," << elem_counts[i][j].second << ")";
	cerr << "\n";
      */
      
      // Use canonical representations
      huff[i] = new Huffman<uint, Elias_int_streamer>(elem_counts[i], true);
      huff[i]->outputDescription(out);
    }
    
    int last = remap[(int)table[0]];
    int c = 1;
    out.writeByte(last);
    for (int i=1; i<size; i++) {
      if (remap[table[i]] == last) {
	++c;
      } else {
	huff[last]->encode(out, c);
	c = 1;
	
	int next = remap[(int)table[i]];

	const bool CONV[4][4] =
	  {{0, 0, 1, 0},
	   {0, 0, 1, 0},
	   {0, 1, 0, 0},
	   {0, 0, 0, 0}};
	out.writeBit(CONV[last][next]);

	last = next;
      }
    }
    huff[last]->encode(out, c);
    out.close();
    
    int num_bytes = (out.num_bits_since_mark()+7)/8;
    cerr << "\n\nSize of run length encoded endgame = " << num_bytes << " bytes.\n"
	 << "\t(num bits = " << out.num_bits_since_mark() << ")\n";
    // Test if above size is correct:
    memset(data+num_bytes, 0, size - num_bytes);
  }

  // VERIFY that the compression worked
  {
    ibstream<array_ibstream> in(data);

    vector<Huffman<uint, Elias_int_streamer> * > huff;
    huff.resize(num_different_values);
    for (int i=0; i<num_different_values; i++)
      huff[i] = new Huffman<uint, Elias_int_streamer>(in);

    int index = 0;
    int last = in.getByte();
    {
      uint c = huff[last]->decode(in);
      while (c--) {
	if (convert_table[last] != table[index]  &&
	    table[index] != dont_care_value) {
	  cerr << "Error on index " << index << ":\n"
	       << (int)convert_table[last] << " = convert_table[" << last << "] != table["
	       << index << "] = " << (int)table[index] << "\n";
	  return;
	}
	index++;
      }
    }
    
    while (index < size) {
      const int INV_CONV[3][2] = {{1,2},{0,2},{0,1}};
      last = INV_CONV[last][(int)(in.getBit())];

      {
	uint c = huff[last]->decode(in);
	while (c--) {
	  if (convert_table[last] != table[index]  &&
	      table[index] != dont_care_value) {
	    cerr << "Error on index " << index << ":\n"
		 << (int)convert_table[last] << " = convert_table[" << last << "] != table["
		 << index << "] = " << (int)table[index] << " (c = " << c << ")\n";
	    return;
	  }
	  index++;
	}
      }
    }
  }
}
