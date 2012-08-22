template <class TYPE>
class HuffTreeNode {
public:
  HuffTreeNode() : element(), left_child(0) {}
  HuffTreeNode(pair<TYPE, float> element_probability) :
    element(element_probability.first), probability(element_probability.second),
    left_child(0) {}
  HuffTreeNode(float probability, int left_child) :
    element(), probability(probability), left_child(left_child) {}

  // used when making huff tree canonical:
  // code_length is equivalent with the distance from the root node (depth).
  char code_length;

  TYPE element;
  float probability;

  // if left_child == 0, then this is a leaf
  // The left and right child is tree[left_child] and tree[left_child+1]
  int left_child;
  bool isLeaf() { return !left_child; }
  bool isInternalNode() { return left_child; }
};

template <class TYPE>
class NodeValue_less {
public:
  bool operator()(const HuffTreeNode<TYPE> & n1, const HuffTreeNode<TYPE> & n2) {
    return n1.probability > n2.probability;
  }
};


// -----------------------------------------------------


template <class TYPE, class ElementStreamer>
Huffman<TYPE, ElementStreamer>::Huffman() : tree(0) {}

template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::init(const vector<pair<TYPE, float> > & distribution, bool makeCanonical) {
  isCanonical = makeCanonical;
  dummy_active = false;
  create(distribution);
}

template <class TYPE, class ElementStreamer>
Huffman<TYPE, ElementStreamer>::Huffman(const vector<pair<TYPE, float> > & distribution, bool makeCanonical) :
  tree(0)
{
  init(distribution, makeCanonica);
}

template <class TYPE>
struct CmpSecondElement {
  bool operator()(const pair<TYPE, int> &p1, const pair<TYPE, int> &p2) const {
    return p1.second < p2.second;
  }
};

template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::init(vector<pair<TYPE, int> > &elem_counts, bool makeCanonical) {
  isCanonical = makeCanonical;

  // Find out which elements occur often enough to be worth storing in the tree.
  // First sort the elements according to their counts.
  CmpSecondElement<TYPE> _cmp_;
  sort(elem_counts.begin(), elem_counts.end(), _cmp_);
  
  /*
  typedef typename vector<pair<TYPE, int> >::iterator I;
  for (I i=elem_counts.begin(); i!=elem_counts.end(); i++) {
    cerr << element_streamer.toString((*i).first) << ": "
	 << (*i).second << "\n";
  }
  */

  // To estimate whether an element occur often enough to be worth storing as part of
  // the description of the tree, we need the value element_streamer.average_bit_size().
  //
  // We start by computing the saving of not storing the least frequently occuring element
  // and then continue to exclude the 2. least, 3. least etc. frequently elements until
  // the estimated saving no longer increase.
  double gain = -2*element_streamer.average_bit_size();//dummy will be stored twice (can be optimized)
  double max_gain = 0.0;
  int insuf_count = 0;
  int index = 0;
  // k_i symbols
  // each with n_i occurances
  int k = 1;
  int n = elem_counts[0].second;
  int total_count = n;
  for (uint i=1; i<elem_counts.size()+1; i++) {
    if (i==elem_counts.size()  ||  elem_counts[i].second != n) {
      // (log2 sum_i(k_i*n_i)) * sum_i(k_i*n_i) - sum(k_i*n_i*log2(n_i)) -
      // element_streamer.bit_size()*sum_i(k_i*n_i) + element_streamer.bit_size()*sum_i(k_i)
      gain += element_streamer.average_bit_size()*k - k*n*log2(n);
      double rest = total_count*(log2(total_count) - element_streamer.average_bit_size());
      if (gain+rest > max_gain) {
	max_gain = gain+rest;
	insuf_count = n;
	index = i;
	// cerr << k << " symbols each occuring " << n << " times" << ". gain = " << max_gain << "\n";
      }

      if (i!=elem_counts.size()) {
	n = elem_counts[i].second;
	k = 1;
	total_count += n;
      }
    } else {
      ++k;
      total_count += n;
    }
  }
  
  // cerr << "Hertil " << insuf_count << " " << index << "\n";

  if (dummy_active = insuf_count) {
    // A non-empty set of elements was not worth storing.
    // A least occuring element (elem_counts[0].first) will now be used to represent
    // the set of these low-frequently occuring symbols.
    dummy = elem_counts[0].first;
    for (int i=0; i<index; i++) {
      ignored_elements[elem_counts[i].first] = true;
    }
  }

  // Let count be the total number of all elements
  int count = 0;
  for (uint i=0; i<elem_counts.size(); i++)
    count += elem_counts[i].second;
  
  //
  double f = 1.0/count;
  vector<pair<TYPE, float> > distribution(elem_counts.size()-index+dummy_active);
  for (uint i=index; i<elem_counts.size(); i++) {
    distribution[i-index] = pair<TYPE, float>(elem_counts[i].first, f*elem_counts[i].second);
    count -= elem_counts[i].second;
  }
  if (dummy_active) {
    // add the special dummy symbol
    distribution[elem_counts.size()-index] = pair<TYPE, float>(dummy, f*count);
  }

  create(distribution);
}

template <class TYPE, class ElementStreamer>
Huffman<TYPE, ElementStreamer>::Huffman(vector<pair<TYPE, int> > &elem_counts, bool makeCanonical) :
  tree(0)
{
  init(elem_counts, makeCanonical);
}

template <class TYPE, class ElementStreamer>
template <class INPUT_MODEL>
void Huffman<TYPE, ElementStreamer>::init(ibstream<INPUT_MODEL> &in) {
  isCanonical = in.getBit();
  dummy_active = in.getBit();
  if (dummy_active) {
    element_streamer.readElement(in, dummy);
  }

  if (isCanonical) {
    int leaves_left = 1;
    int i=0;
    num_diff_elem = 0;
    while (leaves_left) {
      depth_count[i] = in.getEliasNumber();
      // std::cerr << "depth " << i << " count = " << depth_count[i] << "\n";
      leaves_left -= depth_count[i];
      num_diff_elem += depth_count[i];
      leaves_left <<= 1;
      i++;
    }
    tree_size = num_diff_elem + num_diff_elem - 1;
    
    tree = new HuffTreeNode<TYPE>[tree_size];
    int tree_insert = 1;
    int depth = 0;
    // tree[i].left_child will be used to denote depth
    for (int i=0; i<tree_size; i++) {
      while (!depth_count[depth]) ++depth;
      if (tree[i].left_child == depth) {
	tree[i].left_child = 0;
	element_streamer.readElement(in, tree[i].element);
	--depth_count[depth];
      } else {
	// Set the depth of the children
	tree[tree_insert + 0].left_child = tree[i].left_child + 1;
	tree[tree_insert + 1].left_child = tree[i].left_child + 1;
	tree[i].left_child = tree_insert;
	tree_insert += 2;
      }
    }
    // printTree(0, true, 128);
  } else {
    
    num_diff_elem = in.getEliasNumber();
    tree_size = 2*num_diff_elem - 1;
    tree = new HuffTreeNode<TYPE>[tree_size];
    
    // tree[tree_insert_pos] is the first free HuffTreeNode
    int tree_insert_pos = 1;
    recurse_readTreeElements(in, 0, tree_insert_pos);
  }
  
  build_element_codes(0,0,0);
}

template <class TYPE, class ElementStreamer>
template <class INPUT_MODEL>
Huffman<TYPE, ElementStreamer>::Huffman(ibstream<INPUT_MODEL> &in) {
  init(in);
}


template <class TYPE, class ElementStreamer>
template <class OUTPUT_MODEL>
void Huffman<TYPE, ElementStreamer>::outputDescription(obstream<OUTPUT_MODEL> &out) {
  uint old_count = out.num_bits_since_mark();
  out.mark();
  out.writeBit(isCanonical);
  out.writeBit(dummy_active);
  if (dummy_active) {
    element_streamer.writeElement(out, dummy);
  }
  if (isCanonical) {
    int leaves_left = 1;
    int i=0;
    while (leaves_left) {
      out.writeEliasNumber(depth_count[i]);
      leaves_left -= depth_count[i];
      leaves_left <<= 1;
      // std::cerr << "writing depth " << i << " count = " << depth_count[i] << "\n";
      i++;
    }
    recurse_outputTreeElements(out, 0, false);
  } else {
    // std::cerr << "Only a canonical huff tree can be outputtet.\n";
    out.writeEliasNumber(num_diff_elem);
    recurse_outputTreeElements(out, 0, true);
  }
  cerr << "Huff tree size = " << out.num_bits_since_mark() << " bits, including the description of "
       << num_diff_elem << " elements.\n";
  out.mark(old_count + out.num_bits_since_mark());
  printTree(cerr, 0, true, 100);
}

template <class TYPE, class ElementStreamer>
template <class OUTPUT_MODEL>
void Huffman<TYPE, ElementStreamer>::encode(obstream<OUTPUT_MODEL> &out, TYPE element) {
  if (dummy_active  &&  ignored_elements[element]) {
    // This element is not stored as a part of the description of the huffman tree.
    // It will have to be output
    out.writeNBit(element_codes[dummy]);
    element_streamer.writeElement(out, element);
  } else {
    out.writeNBit(element_codes[element]);
  }
}

template <class TYPE, class ElementStreamer>
template <class INPUT_MODEL>
TYPE Huffman<TYPE, ElementStreamer>::decode(ibstream<INPUT_MODEL> &in) {
  int index = 0;
  while (tree[index].isInternalNode())
    index = tree[index].left_child + in.getBit();
  if (dummy_active  &&  tree[index].element==dummy) {
    TYPE result;
    element_streamer.readElement(in, result);
    return result;
  } else {
    return tree[index].element;
  }
}

// --------------- PRIVATE ------------------

template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::
build_huff_tree_from_distribution(const vector<pair<TYPE, float> > & distribution,
				  bool put_smallest_probability_right) {
  num_diff_elem = 0;
  priority_queue<HuffTreeNode<TYPE>, vector<HuffTreeNode<TYPE> >, NodeValue_less<TYPE> > pq;
  typedef typename vector<pair<TYPE, float> >::const_iterator CI;
  for (CI i = distribution.begin(); i !=distribution.end(); i++) {
    // std::cerr << (*i).first << " - " << (*i).second << "\n";
    pq.push(HuffTreeNode<TYPE>(*i));
    num_diff_elem++;
  }
  
  // std::cerr << "num different elements = " << num_diff_elem << "\n";
  int tree_index = tree_size = num_diff_elem + num_diff_elem - 1;
  tree = new HuffTreeNode<TYPE>[tree_size];
  while (true) {
    tree[--tree_index] = pq.top();
    pq.pop();
    
    if (pq.empty()) {
      assert(tree_index == 0);
      break;
    } else {
      tree[--tree_index] = pq.top();
      pq.pop();
      
      if (put_smallest_probability_right  &&
	  tree[tree_index].probability < tree[tree_index + 1].probability)
	swap(tree[tree_index], tree[tree_index + 1]);
      
      pq.push(HuffTreeNode<TYPE>(tree[tree_index].probability + tree[tree_index+1].probability,
				 tree_index));
    }
  }
  
  build_element_codes(0,0,0);
}



template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::create(const vector<pair<TYPE, float> > & distribution) {
  build_huff_tree_from_distribution(distribution, false);
  
  if (isCanonical) {
    for (int i=0; i<32; i++) depth_count[i]=0;
    
    vector<pair<TYPE, float> > new_dist(num_diff_elem);
    int index = 0;
    for (int i=0; i<tree_size; i++)
      if (tree[i].isLeaf()) {
	++depth_count[(int)(tree[i].code_length)];
	float new_prob = 1.0 / ((1<<tree[i].code_length)-1);
	new_dist[index++] = pair<TYPE, float>(tree[i].element, new_prob);
      }
    
    delete tree;
    // The modified probabilities together with putting the nodes with
    // the smallest probability left ensures that the constructed
    // tree will grow in depth from left to right.
    build_huff_tree_from_distribution(new_dist, true);
  }
}

template <class TYPE, class ElementStreamer>
template <class OUTPUT_MODEL>
void Huffman<TYPE, ElementStreamer>::
recurse_outputTreeElements(obstream<OUTPUT_MODEL> &out, int index, bool output_nodeInfo) {
  if (tree[index].left_child) {
    if (output_nodeInfo) out.writeBit(1);
    recurse_outputTreeElements(out, tree[index].left_child, output_nodeInfo);
    recurse_outputTreeElements(out, tree[index].left_child + 1, output_nodeInfo);
  } else {
    if (output_nodeInfo) out.writeBit(0);
    element_streamer.writeElement(out, tree[index].element);
  }
}

template <class TYPE, class ElementStreamer>
template <class INPUT_MODEL>
void Huffman<TYPE, ElementStreamer>::
recurse_readTreeElements(ibstream<INPUT_MODEL> &in, int index, int & tree_insert_pos) {
  if (in.getBit()) {
    // internal node
    tree[index].left_child = tree_insert_pos;
    tree_insert_pos += 2;
    recurse_readTreeElements(in, tree[index].left_child + 0, tree_insert_pos);
    recurse_readTreeElements(in, tree[index].left_child + 1, tree_insert_pos);
  } else {
    element_streamer.readElement(in, tree[index].element);
  }
}

template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::build_element_codes(int index, int code_length, int code) {
  if (tree[index].isInternalNode()) {
    build_element_codes(tree[index].left_child+0, code_length+1, (code<<1)|0);
    build_element_codes(tree[index].left_child+1, code_length+1, (code<<1)|1);
  } else {
    tree[index].code_length = code_length;
    element_codes[tree[index].element] = n_bit(code_length, code);
  }
}


template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::printTree(ostream &os, int index, bool draw_leaves, int max_width) {
  os << "\n";
  WHC whc = getTreeWHC(index, draw_leaves);
  
  // cerr << "(w,h,c) = (" << whc.w << "," << whc.h << "," << whc.c << ")\n";
  
  char *tree_pic = (char *)malloc(sizeof(char)*whc.w*whc.h);
  char **pic = (char **)malloc(sizeof(char *)*whc.h);
  for (int i=0; i<whc.h; i++) {
    pic[i] = &(tree_pic[i*whc.w]);
    
    for (int j=0; j<whc.w; j++)
      pic[i][j] = ' ';
  }
  
  printSubTree(os, pic, index, 0, 0, draw_leaves);
  
  int width;
  string delim;
  if (max_width) {
    width = max_width > whc.w ? whc.w : max_width;
    delim = "#\n";
  } else {
    width = whc.w;
    delim = "\n";
  }
  
  for (int i=0; i<whc.h; i++)
    os << string(pic[i], width) << '\n';
  
  os << "\n";

  free(tree_pic);
  free(pic); 
}

template <class TYPE, class ElementStreamer>
void Huffman<TYPE, ElementStreamer>::
printSubTree(ostream &os, char **pic, int index, int row, int colum, bool draw_leaves) {
  if (tree[index].isLeaf()) {
    if (draw_leaves) {
      string s = element_streamer.toString(tree[index].element);
      for (uint i=0; i<s.length(); i++)
	pic[row + i][colum] = s[i];
    } else {
      pic[row][colum] = 'x';
    }
  } else {
    WHC left_child = getTreeWHC(tree[index].left_child + 0, draw_leaves);
    WHC right_child = getTreeWHC(tree[index].left_child + 1, draw_leaves);
    
    for (int i=left_child.c; i<left_child.w-1; i++)
      pic[row][colum + i] = '_';
    pic[row][colum + left_child.w - 1] = '|';
    for (int i=0; i<right_child.c - 1; i++)
      pic[row][colum + left_child.w + i] = '_';
    
    printSubTree(os, pic, tree[index].left_child + 0, row+1, colum, draw_leaves);
    printSubTree(os, pic, tree[index].left_child + 1, row+1, colum+left_child.w, draw_leaves);
  }
}

template <class TYPE, class ElementStreamer>
WHC Huffman<TYPE, ElementStreamer>::getTreeWHC(int index, bool draw_leaves) {
  if (tree[index].isLeaf()) {
    if (draw_leaves) {
      return WHC(2,element_streamer.toString(tree[index].element).length(),1);
    } else {
      return WHC(2,1,1);
    }
  }
  
  WHC p1 = getTreeWHC(tree[index].left_child + 0, draw_leaves);
  WHC p2 = getTreeWHC(tree[index].left_child + 1, draw_leaves);
  
  return WHC(p1.w+p2.w, 1+(p1.h > p2.h ? p1.h : p2.h), p1.w);
}


//------------- Huffman Codec -----------------

template <class TYPE, class ElementStreamer>
void HuffmanCodec<TYPE, ElementStreamer>::compress(string source, string destination,
						   bool use_canonical) {
  ZerothOrderModel<TYPE, ElementStreamer> model(source);
  int ent = (int)(model.getEntropy() * model.getNumElements());
  cerr << "Total entropy : " << ent << " bits or " << ent/8 << " bytes.\n";

  Huffman<TYPE, ElementStreamer> huff(model.getDistribution(), use_canonical);

  ibstream<file_ibstream> in(source);
  obstream<file_obstream> out(destination);

  huff.outputDescription(&out);
  out.writeArbitraryNumber(model.getNumElements());

  ElementStreamer element_streamer;
  TYPE element;
  while (element_streamer.readElement(&in, element)) {
    huff.encode(&out, element);
  }

  in.close();
  out.close();
}

template <class TYPE, class ElementStreamer>
void HuffmanCodec<TYPE, ElementStreamer>::compress2(string source, string destination,
						    bool use_canonical) {
  ZerothOrderModel<TYPE, ElementStreamer> model(source);
  int ent = (int)(model.getEntropy() * model.getNumElements());
  cerr << "Total entropy : " << ent << " bits or " << ent/8 << " bytes.\n";

  Huffman<TYPE, ElementStreamer> huff(model.getElemCounts(), use_canonical);

  ibstream<file_ibstream> in(source);
  obstream<file_obstream> out(destination);

  huff.outputDescription(&out);
  out.writeArbitraryNumber(model.getNumElements());

  ElementStreamer element_streamer;
  TYPE element;
  while (element_streamer.readElement(&in, element)) {
    huff.encode(&out, element);
  }

  in.close();
  out.close();
}

template <class TYPE, class ElementStreamer>
void HuffmanCodec<TYPE, ElementStreamer>::decompress(string source, string destination) {
  ibstream<file_ibstream> in(source);
  obstream<file_obstream> out(destination);
  
  Huffman<TYPE, ElementStreamer> huff(&in);

  int num_elem = in.getArbitraryNumber();

  ElementStreamer element_streamer;
  TYPE element;
  for (int i=0; i<num_elem; i++) {
    element_streamer.writeElement(&out, huff.decode(&in));
  }

  in.close();
  out.close(false);
}
