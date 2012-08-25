#include "bdd_compression.hxx"

#include "../typedefs.hxx"

#include <assert.h>
#include <vector>
#include <stack>
#include <iostream>

typedef unsigned char uchar;

struct NodePair {
  NodePair() : id(0), left_in(-1), right_out(-1),
	       left_out(0), right_in(0), left_color(false), right_color(false) {}

  bool left_node_connected() const { return left_in != -1; }
  bool right_node_connected() const { return right_out != -1; }

  bool left_out_defined() const { return left_out != 255; }
  bool right_in_defined() const { return right_in != 255; }

  int get_left_out() const {
    assert(left_out_defined());
    return left_in + left_out;
  };
  int get_right_in() const {
    assert(right_in_defined());
    return right_out + right_in;
  };

  void decolor() { left_color = right_color = false; }

  uint id;
  
  // left_in and right_out is indexes into the vectors left_edges and right_edges
  // defined in the main function.
  // if left_node_connected() then left_edges[left_in ... x[ define the incoming
  // edges of the left node. x is the least index for which
  //         left_edges[left_in].sign != left_edges[x].sign
  int left_in, right_out;

  // the left node can have at most one outgoing edge and right_in at most one incoming edge.
  // if left_out_defined() then this edge is left_edges[get_left_out()].
  // Similar for right_in
  uchar left_out, right_in;
  
  bool left_color, right_color;
};

// For each edge in the graph there is 2 Edge stuctures.
// One connected to left nodes and one to the right nodes.
// Edge(left_node).index = index i liste over right_nodes
// Edge(left_node).sign ens for alle edges bundet til left_node
// Edge(left_node).locator = index i forhold til offset i right_node liste over edges
struct Edge {
  Edge() : index(0), sign(false), locator(0) {}

  int index;
  bool sign;

  uchar locator;
};

struct StackItem {
  StackItem() : index(0), edge_num(0), undo_id(0),
		undo_left_out(0), undo_right_in(0) {}
  StackItem(int index, int edge_num, int undo_id) :
    index(index), edge_num(edge_num), undo_id(undo_id),
		undo_left_out(0), undo_right_in(0)  {}
  int index;
  int edge_num;

  int undo_id;
  uchar undo_left_out, undo_right_in;
};

template <class TYPE>
class MyStack {
public:
  MyStack(int default_size = 4) :
    mem_size(default_size), top_index(-1)
  {
    mem = new TYPE[mem_size];
  }
  ~MyStack() { delete[] mem; }

  void push(const TYPE &elem) {
    if (++top_index == mem_size) {
      TYPE *tmp = new TYPE[2*mem_size];
      memcpy(tmp, mem, mem_size*sizeof(TYPE));
      delete[] mem;
      mem = tmp;
      mem_size *= 2;
    }
    mem[top_index] = elem;
  }

  void pop() {
    assert(!empty());
    --top_index;
  }
  
  TYPE &top() { return mem[top_index]; }
  const TYPE &top() const { return mem[top_index]; }

  void clear() { top_index = -1; }

  bool empty() const { return top_index == -1; }

  int size() const { return top_index+1; }

  TYPE &operator[](int index) {
    assert(0<=index  &&  index<=top_index);
    return mem[index];
  }
  const TYPE &operator[](int index) const {
    assert(0<=index  &&  index<=top_index);
    return mem[index];
  }

private:
  TYPE *mem;
  int mem_size;
  int top_index;
};

MyStack<pair<int, int> > id_stack;

bool try_set_id(vector<NodePair> &nodes, const vector<Edge> &left_edges,
		       int index, uint id) {
  if (nodes[index].id != id) {
    id_stack.push(pair<int, int>(index, nodes[index].id));
    nodes[index].id = id;
    while (nodes[index].left_out_defined()) {
      index = left_edges[nodes[index].get_left_out()].index;
      if (nodes[index].id == id) return false;
      id_stack.push(pair<int, int>(index, nodes[index].id));
      nodes[index].id = id;
    }
  }
  return true;
}

void undo_update_id(vector<NodePair> &nodes) {
  while (!id_stack.empty()) {
    //cerr << "Undo: index = " << id_stack.top().first << " set to id = " << id_stack.top().second << '\n';
    nodes[id_stack.top().first].id = id_stack.top().second;
    id_stack.pop();
  }
}

// Returns 
bool try_update_id(vector<NodePair> &nodes, const vector<Edge> &left_edges,
			  MyStack<StackItem> &call_stack) {
  id_stack.clear();
  // Fewest id-changes if call_stack traversed in this order (?)
  for (int i=0; i<call_stack.size(); i++) {
    int left_index = call_stack[i].index;
    int right_index = left_edges[nodes[left_index].left_in + call_stack[i].edge_num].index;
    
    if (!try_set_id(nodes, left_edges, right_index, nodes[left_index].id)) return false;
  }

  return true;
}


void update_changing_path(vector<NodePair> &nodes, const vector<Edge> &left_edges,
				 MyStack<StackItem> &call_stack) {
  for (int i=call_stack.size()-1; i>=0; i--) {
    StackItem &left = call_stack[i];
    
    int left_edge_index = nodes[left.index].left_in + left.edge_num;
    int right_index = left_edges[left_edge_index].index;
    
    // Remember undo info
    left.undo_right_in = nodes[right_index].right_in;
    left.undo_left_out = nodes[left.index].left_out;

    nodes[right_index].right_in = left_edges[left_edge_index].locator;
    nodes[left.index].left_out = left.edge_num;
  }
}

void reverse_update_changing_path(vector<NodePair> &nodes, const vector<Edge> &left_edges,
					 const MyStack<StackItem> &call_stack) {
  for (int i=call_stack.size()-1; i>=0; i--) {
    const StackItem &left = call_stack[i];
    
    int left_edge_index = nodes[left.index].left_in + left.edge_num;
    int right_index = left_edges[left_edge_index].index;
    
    nodes[right_index].right_in = left.undo_right_in;
    nodes[left.index].left_out = left.undo_left_out;
    //cerr << right_index << ' ' << (int)left.undo_right_in << ' ' << (int)left.undo_left_out << '\n';
  }
}

void print(const vector<NodePair> &nodes, const vector<Edge> &left_edges,
	   const MyStack<StackItem> &call_stack) {
  cerr << "Path ";
  for (int ii=0; ii<call_stack.size(); ii++) {
    if (ii) cerr << " -b> ";
    const StackItem &left = call_stack[ii];
    int left_edge_index = nodes[left.index].left_in + left.edge_num;
    int right_index = left_edges[left_edge_index].index;
    cerr << left.index << '(' << nodes[left.index].id << ") -> "
	 << right_index << '(' << nodes[right_index].id << ')';
  }
  cerr << '\n';
}

void print_graph(int shuffle_size, const vector<NodePair> &nodes,
		 const vector<Edge> &left_edges, const vector<Edge> &right_edges) {
  cerr << "GRAPH:\n";
  for (int i=0; i<shuffle_size; i++) {
    cerr << "Left " << i << ": index=" << nodes[i].left_in << ", id = " << nodes[i].id << ", neighbors:";
    if (nodes[i].left_node_connected()) {
      int tmp = nodes[i].left_in;
      for (int j=0; left_edges[tmp].sign == left_edges[tmp+j].sign; j++) {
	if (j == nodes[i].left_out) {
	  cerr << " OUT(" << left_edges[tmp+j].index << ")";
	} else {
	  cerr << ' ' << left_edges[tmp+j].index;
	}
      }
    } else {
      cerr << " none!";
    }
    cerr << '\n';
  }
  for (int i=0; i<shuffle_size; i++) {
    cerr << "Right " << i << ": index=" << nodes[i].right_out << ", neighbors:";
    if (nodes[i].right_node_connected()) {
      int tmp = nodes[i].right_out;
      for (int j=0; right_edges[tmp].sign == right_edges[tmp+j].sign; j++) {
	if (j == nodes[i].right_in) {
	  cerr << " IN(" << right_edges[tmp+j].index << ")";
	} else {
	  cerr << ' ' << right_edges[tmp+j].index;
	}
      }
    } else {
      cerr << " none!";
    }
    cerr << '\n';
  }
}


vector<uint> yalla(int shuffle_size, uint *pairs, int num_pairs, int barrier) {
#ifndef NDEBUG
  // assert(0<=barrier  &&  barrier<shuffle_size); usefull not to restrict
  for (int i=0; i<num_pairs; i++) {
    assert(0 <= pairs[2*i]  &&  pairs[2*i] < (uint)shuffle_size);
    assert(0 <= pairs[2*i+1]  &&  pairs[2*i+1] < (uint)shuffle_size);
    //assert(pairs[2*i] != pairs[2*i+1]);
  }
#endif

  vector<NodePair> nodes(shuffle_size);
  
  // Count num_edges
  // Count number of edges attached to each node (max allowed = 254)
  int num_edges = 0;
  for (int i=0; i<num_pairs; i++) {
    int left = pairs[2*i];
    int right = pairs[2*i+1];
    assert(left < shuffle_size);
    assert(right < shuffle_size);
    if (left != right  &&
	((left < barrier  &&  right < barrier) || (left >= barrier  &&  right >= barrier))  &&
	nodes[left].left_out < 254  &&  nodes[right].right_in < 254) {
      num_edges++;
      ++nodes[left].left_out;
      ++nodes[right].right_in;
    }
  }

  //cerr << "Num edges connected to nodes:\n";
  //for (int i=0; i<shuffle_size; i++)
  //  cerr << "left: " << (int)nodes[i].left_out << ", right: " << (int)nodes[i].right_in << '\n';

  vector<Edge> left_edges(num_edges+1);
  vector<Edge> right_edges(num_edges+1);

  // Set indexes nodes[].left_in and nodes[].right_out
  // Set left_sign and right_sign
  {
    int left_index = 0;
    int right_index = 0;
    bool last_left_sign = true;
    bool last_right_sign = true;
    for (int i=0; i<shuffle_size; i++) {
      nodes[i].id = i;

      if (nodes[i].left_out) {
	nodes[i].left_in = left_index;

	last_left_sign ^= 1;
	for (int j=0; j<nodes[i].left_out; j++)
	  left_edges[left_index + j].sign = last_left_sign;

	left_index += nodes[i].left_out;
	nodes[i].left_out = 0;

      } else {
	nodes[i].left_in = -1;
      }

      if (nodes[i].right_in) {
	nodes[i].right_out = right_index;

	last_right_sign ^= 1;
	for (int j=0; j<nodes[i].right_in; j++)
	  right_edges[right_index + j].sign = last_right_sign;

	right_index += nodes[i].right_in;
	nodes[i].right_in = 0;

      } else {
	nodes[i].right_out = -1;
      }
    }

    // Complete the sign property
    left_edges[left_index].sign = (last_left_sign^=1);
    right_edges[right_index].sign = (last_right_sign^=1);
    
    assert(left_index == num_edges);
    assert(right_index == num_edges);
  }

  if (false) {
    cerr << "SIGN:\nLeft edges:\n";
    for (int i=0; i<=num_edges; i++)
      cerr << (left_edges[i].sign ? 'T' : 'F');
    cerr << "\nRight edges:\n";
    for (int i=0; i<=num_edges; i++)
      cerr << (right_edges[i].sign ? 'T' : 'F');
    cerr << '\n';
  }
  
  // Set edges
  for (int i=0; i<num_pairs; i++) {
    int left = pairs[2*i];
    int right = pairs[2*i+1];
    if (left != right  &&
	((left < barrier  &&  right < barrier) || (left >= barrier  &&  right >= barrier))  &&
	nodes[left].left_out < 254  &&  nodes[right].right_in < 254) {

      {
	Edge &edge = left_edges[nodes[left].get_left_out()];
	edge.index = right;
	edge.locator = nodes[right].right_in;
      }
      {
	Edge &edge = right_edges[nodes[right].get_right_in()];
	edge.index = left;
	edge.locator = nodes[left].left_out;
      }

      nodes[right].right_in++;
      nodes[left].left_out++;
    }
  }

  // Clear nodes[].left_out, nodes[].right_in
  for (int i=0; i<shuffle_size; i++)
    nodes[i].left_out = nodes[i].right_in = 255;

  if (false) print_graph(shuffle_size, nodes, left_edges, right_edges);



  // Now the graph is constructed

  int compression = 0;

  bool progress = true;
  while (progress) {
    progress = false;
 
    // Color graph white
    //cerr << "Resetting graph color to white (compression = " << compression << ")\n";
    for (int i=0; i<shuffle_size; i++)
      nodes[i].decolor();

    MyStack<StackItem> call_stack;
    for (int i=0; i<shuffle_size; i++)
      if (nodes[i].left_node_connected()  &&
	  !nodes[i].left_out_defined()  &&
	  !nodes[i].left_color) {
	call_stack.push(StackItem(i, 0, 0));
	int num_undoings = 0;

	while (!call_stack.empty()) {
	  //cerr << "Ok, " << call_stack[i].index << '\n';

	  int left_index = call_stack.top().index;
	  int left_in_offset = nodes[left_index].left_in;
	  int *left_in_index = &(call_stack.top().edge_num);
	  
	  while (left_edges[left_in_offset].sign == left_edges[left_in_offset + *left_in_index].sign) {
	    int right_index = left_edges[left_in_offset + *left_in_index].index;
	    if (!nodes[right_index].right_color  &&
		// ID stuff:
		nodes[left_index].id != nodes[right_index].id) {

	      nodes[right_index].right_color = true;
	      
	      if (!nodes[right_index].right_in_defined()) {

		update_changing_path(nodes, left_edges, call_stack);
		//print(nodes, left_edges, call_stack);

		if (try_update_id(nodes, left_edges, call_stack)) {
		  // Turning path found!
		  progress = true;
		  ++compression;

		  //cerr << "Updated "; print(nodes, left_edges, call_stack);

		  call_stack.clear();

#ifndef NDEBUG
		  // Tjek om id'erne er korrekte
		  for (int j=0; j<shuffle_size; j++) {
		    if (nodes[j].right_in_defined()) {
		      int in_index = right_edges[nodes[j].get_right_in()].index;
		      assert(nodes[j].id == nodes[in_index].id);
		      assert(nodes[in_index].left_out_defined());
		      if (left_edges[nodes[in_index].get_left_out()].index != j) {
			cerr << "! " << in_index << ' ' << left_edges[nodes[in_index].get_left_out()].index
			     << ' ' << j << '\n';
			assert(0);
		      }
		    } else {
		      assert(nodes[j].id == (uint)j);
		    }
		  }
		  
		  // Tjek om der er cykler
		  for (int j=0; j<shuffle_size; j++)
		    if (nodes[j].left_out_defined()) {
		      int jj = j;
		      while (nodes[jj].left_out_defined()) {
			jj = left_edges[nodes[jj].get_left_out()].index;
			if (j == jj) {
			  int ii = j;
			  cerr << "ARHG: " << ii;
			  while (nodes[ii].left_out_defined()) {
			    ii = left_edges[nodes[ii].get_left_out()].index;
			    cerr << " -> " << ii;
			    assert(j != ii);
			  }
			}
		      }
		    }
#endif
		  // Hop ud af while-løkken
		  break;

		} else {
		  //cerr << "\n\nUndoing path change in conflict with id!\n";
		  //print(nodes, left_edges, call_stack);
		  undo_update_id(nodes);
		  //print(nodes, left_edges, call_stack);

		  //print_graph(shuffle_size, nodes, left_edges, right_edges) ;
		  reverse_update_changing_path(nodes, left_edges, call_stack);
		  //print_graph(shuffle_size, nodes, left_edges, right_edges) ;
		  //cerr << "\n\n";

		  if (++num_undoings == 2) {
		    // Discard
		    // cerr << "num_undoings too large (2), discarding\n";
		    call_stack.clear();
		    break;
		  }

#ifndef NDEBUG
		  // Tjek om id'erne er korrekte
		  for (int j=0; j<shuffle_size; j++) {
		    if (nodes[j].right_in_defined()) {
		      int in_index = right_edges[nodes[j].get_right_in()].index;
		      assert(nodes[j].id == nodes[in_index].id);
		      assert(nodes[in_index].left_out_defined());
		      if (left_edges[nodes[in_index].get_left_out()].index != j) {
			cerr << "! " << in_index << '(' << nodes[in_index].id << ')'
			     << ' ' << left_edges[nodes[in_index].get_left_out()].index << '('
			     << nodes[left_edges[nodes[in_index].get_left_out()].index].id << ')'
			     << ' ' << j << '(' << nodes[j].id << ')' << '\n';
			assert(0);
		      }
		    } else {
		      assert(nodes[j].id == (uint)j);
		    }
		  }
		  
		  // Tjek om der er cykler
		  for (int j=0; j<shuffle_size; j++)
		    if (nodes[j].left_out_defined()) {
		      int jj = j;
		      while (nodes[jj].left_out_defined()) {
			jj = left_edges[nodes[jj].get_left_out()].index;
			if (j == jj) {
			  int ii = j;
			  cerr << "ARHG: " << ii;
			  while (nodes[ii].left_out_defined()) {
			    ii = left_edges[nodes[ii].get_left_out()].index;
			    cerr << " -> " << ii;
			    assert(j != ii);
			  }
			}
		      }
		    }
#endif
		}

	      } else {
		// right node has an incoming edge

		int test_left_index = right_edges[nodes[right_index].get_right_in()].index;
		if (!nodes[test_left_index].left_color) {
		  nodes[test_left_index].left_color = true;
		  
		  if (nodes[test_left_index].left_node_connected()) {
		    left_index = test_left_index;

		    //cerr << "push(" << left_index << ",0)\n";
		    call_stack.push(StackItem(left_index, 0, nodes[right_index].id));

		    left_in_offset = nodes[left_index].left_in;
		    left_in_index = &(call_stack.top().edge_num);
		  }
		}
	      }
	    }

	    (*left_in_index)++;
	  }

	  if (!call_stack.empty()) call_stack.pop();
	}
      }
  }

  // Now an optimal (?) solution is found
  if (false) { // print solution
    cerr << "Solution:\n";
    for (int i=0; i<shuffle_size; i++)
      if (nodes[i].left_out_defined())
	cerr << "Kant fra " << i << '(' << nodes[i].id << ") til "
	     << left_edges[nodes[i].get_left_out()].index << '('
	     << nodes[left_edges[nodes[i].get_left_out()].index].id << ")\n";
  }
  if (false) {
    cerr << "ID stuff:\n";
    for (int i=0; i<shuffle_size; i++)
      cerr << i << " -> " << nodes[i].id << '\n';
  }

  // Create permutation
  vector<uint> result(shuffle_size);
  int p = 0;
  for (int i=0; i<shuffle_size; i++)
    if (nodes[i].id != 0xFFFFFFFF) {
      int index = nodes[i].id;
      assert(0<=index  &&  index<shuffle_size);
      // Byt om paa nedenstaaende 2 linjer (ogsaa lidt laengere nede) for
      // at faa den inverse permutation
      //result[p++] = index;
      result[index] = p++;
      nodes[index].id = 0xFFFFFFFF;
      while (nodes[index].left_out_defined()) {
	index = left_edges[nodes[index].get_left_out()].index;
	assert(0<=index  &&  index<shuffle_size);
	//result[p++] = index;
	result[index] = p++;
	if (nodes[index].id == 0xFFFFFFFF) {
	  //cerr << "ARGH:\n" << index << ',' << p << ',' << shuffle_size << ',' << i << '\n';
	}
	nodes[index].id = 0xFFFFFFFF;
      }
    }
  if (p != shuffle_size) {
    cerr << "(p,shuffle_size)=(" << p << ',' << shuffle_size << ")\n";
    assert(0);
  }

  if (false) { // print permutation
    cerr << "Permutation:\n";
    for (int i=0; i<shuffle_size; i++)
      cerr << result[i] << ' ';
    cerr << '\n';
  }

  //cerr << "Compression: " << compression << " of max " << num_pairs << '\n';

  return result;
}




void test_yalla() {
  const int N = 7;
  uint pairs[2*N];

  int index = 0;
  pairs[index++] = 0; pairs[index++] = 0;
  pairs[index++] = 1; pairs[index++] = 3;
  pairs[index++] = 3; pairs[index++] = 7;
  pairs[index++] = 6; pairs[index++] = 5;
  pairs[index++] = 7; pairs[index++] = 2;
  pairs[index++] = 6; pairs[index++] = 4;
  pairs[index++] = 4; pairs[index++] = 5;

  vector<uint> ya = yalla(8, pairs, N, 8);
  
  cerr << "test_yalla:\n";
  for (uint i=0; i<ya.size(); i++)
    cerr << " " << ya[i];
  cerr << "\n";
}
