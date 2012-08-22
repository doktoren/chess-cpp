#ifndef _TREE_
#define _TREE_

#include "board.hxx"

class Tree;

class TempMoveList {
public:
  TempMoveList(pair<Move, int> *offset) offset(offset) {}
  void push();

  pair<Move, int> *offset;
  int count;
}

// todo: make a template
class MyContainer {
  MyContainer(int log_block_size = 10) : log_block_size(log_block_size) {
    block_size = 1 << log_block_size;
    count = 0;
    mem = first_block = (char *)(malloc(sizeof(char *) + block_size * sizeof(int)) + sizeof(char *));
    ((char **)mem)[-1] = 0;
  }
  ~MyContainer() {
    while (first_block) {
      char *tmp = ((char **)first_block)[-1];
      delete[] &(((char **)first_block)[-1]);
      first_block = tmp;
    }
  }
  void clear() {
    mem = first_block;
    count = 0;
  }

  int *allocate() {
    if (count & block_size) {
      // Allocate an extra block
      if (*((char **)mem)) {
	// next block already allocated
	mem = *((char **)mem);
      } else {
	char *next = (char *)malloc(sizeof(char *) + block_size * sizeof(int));
	*((char **)mem) = next;
	((char **)mem)[-1] = 0;
	mem = next;
      }
      count = 0;
    }
    return &(mem[count++]);
  }
  int *allocate(int num) {
    assert(num <= count);
    if (count+num >= block_size) {
      // Allocate an extra block (discard any remaining unused space in this on)
      if (*((char **)mem)) {
	// next block already allocated
	mem = *((char **)mem);
      } else {
	char *next = (char *)malloc(sizeof(char *) + block_size * sizeof(int));
	*((char **)mem) = next;
	((char **)mem)[-1] = 0;
	mem = next;
      }
      count = 0;
    }
    return &(mem[count+=num]);
  }
  
  int log_block_size, block_size;
  int count;
  int *first_block, *mem;
};


class Node;

struct ParentStruct {
  ParentStruct(Node *parent) : parent(parent), next(0) {}
  Node *parent;
  ParentStruct *next;
};

class ChildrenStuff;

class Node {
public:
  Node() : parentStruct(0) {}
  Node(ParentStruct parents, int value, uchar num_moves, uchar eval_type, uchar depth) :
    parents(parents), value(value), num_moves(num_moves), eval_type(eval_type),
    depth(depth) {}
  Node(const Node& node) :
    parents(node.parents), depth(node.depth), num_moves(node.num_moves),
    eval_type(node.eval_type), depth(node.depth) {}

  ParentStruct parents;
  int value;
  uchar num_moves;
  uchar eval_type;
  uchar depth;

  bool is_leaf;
  ChildrenStuff *children_stuff;
};

class ChildrenStuff {
  ChildrenStuff(Tree *t, Board2 &b, Node *node) {
    Move move = b.moves();
    pair<Move, int> *tmp_list = t->get_temp_list();
    int count = 0;
    while (b.next_move(move)) {
      tmp_list[count] = pair<Move, int>(move, b.table[b.hash_value_after_move(move)]);
      if (tmp_list[count].second == 0) {
	tmp_list[count].second = t->allocateNode();
      }
      // todo: more stuff
      count++;
    }
    
    // 
    
    moves = t->allocateMoveList(count);
    for (int i=0; i<count; i++) moves[i] = tmp_list[i];
  }

  pair<Move, int> *moves;
};

class Tree {
public:
  Tree() {}

  Node *init_search() {
    parent_structs.clear();
    nodes.clear();
    children_stuffs.clear();
    move_pool.clear();
    tmp_move_list = allocateMoveList(128);

    root = allocateNode();
    *root = Node();
    return root;
  }

  ParentStruct *allocateParentStruct() { return parent_structs.allocate(); }
  ParentStruct *allocateNode() { return nodes.allocate(); }
  ParentStruct *allocateChildrenStuff() { return children_stuffs.allocate(); }
  ParentStruct *allocateMoveList(int num) { return move_pool.allocate(num); }
  pair<Move, int> *get_temp_list() { return tmp_move_list; }
private:
  friend class Tree;

  Node *root;

  MyContainer<ParentStruct> parent_structs;
  MyContainer<Node> nodes;
  MyContainer<ChildrenStuff> children_stuffs;
  MyContainer<pair<Move, int> > move_pool;

  pair<Move, int> *tmp_move_list;
};

#endif
