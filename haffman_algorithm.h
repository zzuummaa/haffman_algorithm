#ifndef HAFFMAN_ALGORITHM_HAFFMAN_ALGORITHM_H
#define HAFFMAN_ALGORITHM_HAFFMAN_ALGORITHM_H

#include <array>
#include <bitset>
#include <vector>
#include <algorithm>
#include <ostream>
#include <istream>

typedef std::vector<std::pair<uint8_t, double>> ByteFrequencies;

struct NodeContent {
	double freq;
	uint8_t val;
};

struct Node {
	Node* parent;
	Node* left;
	Node* right;
	NodeContent content;

	Node() = default;
};

template<size_t Nb>
class bit_set_counted : public std::bitset<Nb> {
public:
	size_t count = 0;

	bool is_full() {
		return count == this->size();
	}
};

bool is_node_freq_ge(Node* a, Node* b);

class HaffmanEncoder {
	std::vector<Node> buffer;
	std::vector<Node*> char_nodes;
	Node* top_node;

	Node* create_leaf_node(const NodeContent& content);

	Node* create_parent_node(Node* left_node,  Node* right_node);

public:
	HaffmanEncoder();
	explicit HaffmanEncoder(const ByteFrequencies& byte_freq);

	void build(const ByteFrequencies& byte_freq);

	template<size_t Nb>
	Node* encode(Node* node, bit_set_counted<Nb>& out) {
		if (node == nullptr) return nullptr;
		while (node->parent != nullptr && out.count < Nb) {
			out[out.count] = node->parent->left == node;
			out.count++;
			node = node->parent;
		}
		return node->parent;
	}

	Node* node_by_char(uint8_t c);

	friend std::ostream& operator<<(std::ostream& os, const HaffmanEncoder& encoder);
	friend std::istream& operator>>(std::istream& is, HaffmanEncoder& encoder);

	int print_encoding_info(std::ostream& os);
};

ByteFrequencies create_byte_frequencies(std::array<size_t, 256>& char_counts, size_t file_size);

#endif //HAFFMAN_ALGORITHM_HAFFMAN_ALGORITHM_H
