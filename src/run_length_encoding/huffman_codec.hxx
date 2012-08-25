#ifndef _HUFFMAN_CODEC_
#define _HUFFMAN_CODEC_

#include <map>
#include <queue>
#include <iostream>
#include <cmath>

#include "PredictionModel.hxx"
#include "bit_stream.hxx"
#include "Codec.hxx"

template <class TYPE>
class HuffTreeNode;

struct WHC {
  WHC() : w(0), h(0), c(0) {}
  WHC(int w, int h, int c) : w(w), h(h), c(c) {}
  int w,h,c;
};


template <class TYPE, class ElementStreamer = DefaultElementStreamer<TYPE> >
class Huffman {
public:
  Huffman();
  ~Huffman() { delete tree; }

  void init(const vector<pair<TYPE, float> > &distribution, bool makeCanonical = false);
  Huffman(const vector<pair<TYPE, float> > &distribution, bool makeCanonical = false);

  // The vector elem_counts will be sorted with *.second in ascending order
  void init(vector<pair<TYPE, int> > &elem_counts, bool makeCanonical = false);
  Huffman(vector<pair<TYPE, int> > &elem_counts, bool makeCanonical = false);
  
  template<class INPUT_MODEL>
  Huffman(ibstream<INPUT_MODEL> &in);
  template<class INPUT_MODEL>
  void init(ibstream<INPUT_MODEL> &in);

  // This description is what is being read by the constructor.
  template<class OUTPUT_MODEL>
  void outputDescription(obstream<OUTPUT_MODEL> &out);

  template<class OUTPUT_MODEL>
  inline void encode(obstream<OUTPUT_MODEL> &out, TYPE element);
  
  template<class INPUT_MODEL>
  inline TYPE decode(ibstream<INPUT_MODEL> &in);

private:
  void create(const vector<pair<TYPE, float> > & distribution);

  template<class OUTPUT_MODEL>
  void recurse_outputTreeElements(obstream<OUTPUT_MODEL> &out, int index, bool output_nodeInfo);
  
  template<class INPUT_MODEL>
  void recurse_readTreeElements(ibstream<INPUT_MODEL> &in, int index, int & tree_insert_pos);
  void build_element_codes(int index, int code_length, int code);
  void build_huff_tree_from_distribution(const vector<pair<TYPE, float> > & distribution,
					 bool put_smallest_probability_right);

  //     ___|___
  //    |_     _|_
  //   x  |   |   |_
  //     x x x x x  |
  //               x x
  void printTree(ostream &os, int index, bool draw_leaves, int max_width = 0);
  void printSubTree(ostream &os, char **pic, int index, int row, int colum, bool draw_leaves);
  WHC getTreeWHC(int index, bool draw_leaves);

  //------- Variables ----------

  ElementStreamer element_streamer;

  bool isCanonical;
  // depth_count only set if isCanonical
  int depth_count[32];

  int num_diff_elem;
  HuffTreeNode<TYPE> *tree;
  int tree_size; // = 2*num_diff_elem - 1

  map<TYPE, n_bit, ElementStreamer> element_codes;

  // The constructor examines if some elements are not worth storing in the
  // description of the tree. It uses element_streamer.average_bit_size() as an estimate.
  // If this is set to 0 then all elements will be included in the description.
  // If this is set very high, then (almost) no elements will be included in the description.
  //
  // if dummy_active then the dummy element represent NoElement
  // (represented by a least occuring element)
  bool dummy_active;
  TYPE dummy;
  map<TYPE, bool, ElementStreamer> ignored_elements;
};


//------------- Huffman Codec -----------------


template <class TYPE, class ElementStreamer = DefaultElementStreamer<TYPE> >
class HuffmanCodec : public Codec {
public:
  void compress(string source, string destination, bool use_canonical);
  // Compress 2 uses element counts instead of probabilities, and can
  // hereby avoid storing a large huff tree...
  void compress2(string source, string destination, bool use_canonical);
  void decompress(string source, string destination);
};


// ---------------------

template <class TYPE, class ElementStreamer = DefaultElementStreamer<TYPE> >
class HuffmanCodec2 {
public:
  HuffmanCodec2();
  void addSymbol(TYPE &element);
  int getCount() { return count; }

  template<class OUTPUT_MODEL>
  void outputAll(obstream<OUTPUT_MODEL> &out);

  bool outputNext();
private:
  int count;
  vector<TYPE> symbols;
  map<TYPE, int> symbol_count;
};

template <class TYPE, class ElementStreamer>
HuffmanCodec2<TYPE, ElementStreamer>::HuffmanCodec2() : count(0) {}

template <class TYPE, class ElementStreamer>
void HuffmanCodec2<TYPE, ElementStreamer>::addSymbol(TYPE &element) {
  // cerr << "--- " << element << " ---\n";

  symbols.push_back(element);
  ++symbol_count[element];
  ++count;
}

template <class TYPE, class ElementStreamer>
template <class OUTPUT_MODEL>
void HuffmanCodec2<TYPE, ElementStreamer>::outputAll(obstream<OUTPUT_MODEL> &out) {
  vector<pair<TYPE, int> > sc(symbol_count.size());
  int i=0;
  typedef typename map<TYPE, int>::const_iterator CI;
  for (CI ci=symbol_count.begin(); ci!=symbol_count.end(); ci++) {
    sc[i++] = *ci;//pair<TYPE, int>((*ci).first, (*ci).second);
  }

  Huffman<TYPE, ElementStreamer> huff(sc, true);
  huff.outputDescription(out);

  for (i=0; i<count; i++) {
    huff.encode(out, symbols[i]);
  }
  
  //----------- Reset stuff -------------
  count = 0;
  symbols = vector<TYPE>();
  symbol_count = map<TYPE, int>();
}


#include "huffman_codec_templ.hxx"

#endif
