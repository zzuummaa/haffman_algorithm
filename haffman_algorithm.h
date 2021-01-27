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
	size_t pos = 0;
	size_t count = 0;

	bool is_full() {
		return count == this->size();
	}
};

template<std::size_t Nb>
void copy_bits(std::bitset<Nb> &b, size_t begin_i, size_t end_i, size_t finish_i) {
	for(size_t i = 0; i < (end_i - begin_i); ++i) {
		b[i + finish_i] = b[i + begin_i];
	}
}

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
		while (out.count < Nb) {
			if (node->parent == nullptr) return nullptr;
			out[out.count] = node->parent->left == node;
			out.count++;
			node = node->parent;
		}
		return node;
	}

	template<size_t Nb>
	Node* decode(Node* node, bit_set_counted<Nb>& in) {
		if (node == nullptr) return nullptr;
		while (in.pos < in.count) {
			if (in.test(in.pos)) {
				if (node->left == nullptr) return node;
				node = node->left;
			} else {
				if (node->right == nullptr) return node;
				node = node->right;
			}
			in.pos++;
		}
		return node;
	}

	Node* node_by_char(uint8_t c);
	bool is_leaf(Node* node);
	Node* top();

	friend std::ostream& operator<<(std::ostream& os, const HaffmanEncoder& encoder);
	friend std::istream& operator>>(std::istream& is, HaffmanEncoder& encoder);

	int print_encoding_info(std::ostream& os);
};

ByteFrequencies create_byte_frequencies(std::array<size_t, 256>& char_counts, size_t file_size);

#endif //HAFFMAN_ALGORITHM_HAFFMAN_ALGORITHM_H
