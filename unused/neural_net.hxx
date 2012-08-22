#include <vector>

// To do: some locator stuff

using namespace std;

struct Node;
struct Edge;

struct NodeID {
  NodeID() : id(0) {}
  NodeID(int id) : id(id) {}
  int id;
};

// The operations that modifies the structure of the neural net
// are pretty slow. Evaluation and learning are rather efficient,
// but not optimal. If short indexing are replaced by int indexing,
// some speed increase would probably occur at the expence of 50% more size.
class NeuralNet {
public:
  NeuralNet(int max_size);
  ~NeuralNet();

  int getNumNodes() { return num_nodes; }
  int getNumEdges() { return num_edges; }

  short find_pos(NodeID node_id);

  // insert_node returns an unique identifier for that node
  NodeID insert_node(short pos);
  void insert_edge(short from_node, short to_node, float weight);
  void insert_edge(NodeID from_node, NodeID to_node, float weight) {
    insert_edge(find_pos(from_node), find_pos(to_node), weight);
  }

  // If you need to remove a node that has edges attached to it,
  // remove_node will remove this as well (and much more efficient
  // than calling remove_edge for each of these)
  void remove_node(short pos);
  void remove_node(NodeID node_id) { remove_node(find_pos(node_id)); }
  void remove_edge(short from_node, short to_node);
  void remove_edge(NodeID from_node, NodeID to_node) {
    remove_edge(find_pos(from_node), find_pos(to_node));
  }

  float evaluate(vector<float> input);
  void back_propagate(float desired_result, float learning_rate);

private:
  NodeID get_unique_node_id();

  int max_size;
  Node *nodes;
  Edge *edges;

  int num_nodes, num_edges;
};
