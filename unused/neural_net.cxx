// g++ neural_net.cxx -o test

#include "neural_net.hxx"

struct Node {
  Node() : in_edges(0), out_edges(0), node_id() {}

  short in_edges;
  short out_edges;

  NodeID node_id;
};

struct Edge {
  Edge() : weight(0), from_node(0), to_node(0),
	   next_in_edge(0), next_out_edge(0) {}
  
  float weight;
  short from_node, to_node;

  short next_in_edge, next_out_edge;

  // Used during computation:
  float value;
};

NeuralNet::NeuralNet(int max_size) {
  assert(max_size < 0x7FFF);
  max_size = max_size+1;
  nodes = new Node[max_size];
  edges = new Edge[max_size];

  // To begin with, only the output node exists
  num_nodes = 1;
  // and no edges
  num_edges = 0;
  
  // use edges[0] and edges.next_in_node as a list representing the
  // unused occurances of edges
  edges[0].next_in_edge = 1;
  for (int i=1; i<max_size; i++)
    edges[i].next_in_edge = i+1;
}

NeuralNet::~NeuralNet() {
  delete nodes, edges;
}

short NeuralNet::find_pos(NodeID node_id) {
  for (int i=1; i<=num_nodes; i++)
    if (nodes[i].node_id.id == node_id.id)
      return i;
  assert(0);
}

NodeID NeuralNet::insert_node(short pos) {
  // is pos legal?
  assert(1 <= pos  &&  pos <= num_nodes);
  // are there any unused nodes left?
  assert(num_nodes < max_size-1);
  
  // Move rest of node list one entry
  ++num_nodes;
  for (int i=num_nodes; i>pos; i--)
    nodes[i] = nodes[i-1];
  
  // Update the edges node references
  // This might be optimized...
  for (int i=1; i<max_size; i++) {
    if (edges[i].from_node >= pos) ++edges[i].from_node;
    if (edges[i].to_node >= pos) ++edges[i].to_node;
  }
  
  nodes[pos] = Node();
  nodes[pos].node_id = get_unique_node_id();
}

void NeuralNet::remove_node(short pos) {
  // is pos legal?
  assert(1 <= pos  &&  pos <= num_nodes);
  
  short edge;
  
  // Remove ingoing edges
  edge = nodes[pos].in_edges;
  while (edge) {
    short remember_edge = edge;
    edge = edges[edge].next_in_edge;
    
    // Remove this edge
    edges[remember_edge].next_in_edge = edges[0].next_in_edge;
    edges[0].next_in_edge = remember_edge;
    --num_edges;
  }
  
  // Remove outgoing edges (only first line differs)
  edge = nodes[pos].out_edges;
  while (edge) {
    short remember_edge = edge;
    edge = edges[edge].next_in_edge;
      
    // Remove this edge
    edges[remember_edge].next_in_edge = edges[0].next_in_edge;
    edges[0].next_in_edge = remember_edge;
    --num_edges;
  }
  
  // Move rest of node list one entry
  for (int i=pos; i<num_nodes; i--)
    nodes[i] = nodes[i+1];
  --num_nodes;
  
  // Update the edges node references
  // This might be optimized...
  for (int i=1; i<max_size; i++) {
    if (edges[i].from_node > pos) --edges[i].from_node;
    if (edges[i].to_node > pos) --edges[i].to_node;
  }
}

void NeuralNet::insert_edge(short from_node, short to_node, float weight) {
  // Check from_node and to_node
  assert(1<=from_node  &&  from_node<=num_nodes  &&
	 1<=to_node  &&  to_node<=num_nodes);
  
  // Are there any unused edges left?
  assert(edges[0].next_in_edge);
  
  { // Assure that edge is not already present
    short edge = nodes[from_node].out_edges;
    while (edge) {
      assert(edges[edge].to_node != to_node);
      edge = edges[edge].next_out_edge;
    }
  }
  
  short edge = edges[0].next_in_edge;
  edges[0].next_in_edge = edges[edge].next_in_edge;
  
  edges[edge].from_node = from_node;
  edges[edge].to_node = to_node;
  edges[edge].weight = weight;
  
  edges[edge].next_out_edge = nodes[from_node].out_edges;
  nodes[from_node].out_edges = edges[edge].next_out_edge;
  
  edges[edge].next_in_edge = nodes[to_node].in_edges;
  nodes[to_node].in_edges = edges[edge].next_in_edge;
  
  ++num_edges;
}

void NeuralNet::remove_edge(short from_node, short to_node) {
  // Check from_node and to_node
  assert(1<=from_node  &&  from_node<=num_nodes  &&
	 1<=to_node  &&  to_node<=num_nodes  &&
	 nodes[from_node].out_edges  &&  nodes[to_node].in_edges);
  
  short removed_edge;
  
  // Remove the edge from the source node:
  short edge = nodes[from_node].out_edges;
  if (edges[edge].to_node == to_node) {
    // edge found!
    nodes[from_node].out_edges = edges[edge].next_out_edge;
    removed_edge = edge;
  } else {
    short last_edge = edge;
    edge = edges[edge].next_out_edge;
    while (edge) {
      if (edges[edge].to_node == to_node) {
	// edge found
	edges[last_edge].next_out_edge = edges[edge].next_out_edge;
	removed_edge = edge;
	break;
      }
      last_edge = edge;
      edge = edges[edge].next_out_edge;
    }
    assert(edge); // edge not found!
  }
  
  // Remove the edge from the destination node:
  edge = nodes[to_node].in_edges;
  if (edges[edge].from_node == from_node) {
    // edge found!
    nodes[to_node].in_edges = edges[edge].next_in_edge;
  } else {
    short last_edge = edge;
    edge = edges[edge].next_in_edge;
    while (edge) {
      if (edges[edge].from_node == from_node) {
	// edge found
	edges[last_edge].next_in_edge = edges[edge].next_in_edge;
	break;
      }
      last_edge = edge;
      edge = edges[edge].next_in_edge;
    }
    assert(edge); // edge not found!
  }
  
  // Remove the edges itself
  edges[removed_edge].next_in_edge = edges[0].next_in_edge;
  edges[0].next_in_edge = removed_edge;
  --num_edges;
}

float NeuralNet::evaluate(vector<float> input) {
  return 0; // to do
}

void NeuralNet::back_propagate(float desired_result, float learning_rate) {
  return;
}

// -------- PRIVATE --------

NodeID NeuralNet::get_unique_node_id() {
  bool id_acceptet;
  int id;
  do {
    id_acceptet = true;
    id = ((rand() << 16)^rand());
    for (int i=1; i<num_nodes; i++)
      if (nodes[i].node_id.id == id) {
	id_acceptet = false;
	break;
      }
  } while (!id_acceptet);
  return NodeID(id);
}


//----------------------------------------------------------------

int main() {
  
}
