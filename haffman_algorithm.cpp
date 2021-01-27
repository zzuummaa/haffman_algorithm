#include "haffman_algorithm.h"

Node *HaffmanEncoder::create_leaf_node(const NodeContent &content) {
	if (buffer.capacity() == buffer.size()) return nullptr;

	buffer.emplace_back();
	Node* node = &buffer.back();
	node->content = content;

	return node;
}

Node *HaffmanEncoder::create_parent_node(Node *left_node, Node *right_node) {
	if (buffer.size() >= buffer.capacity()) return nullptr;

	buffer.emplace_back();
	Node* node = &buffer.back();
	node->content.freq = left_node->content.freq + right_node->content.freq;
	node->left = left_node;
	node->right = right_node;
	left_node->parent = node;
	right_node->parent = node;

	return node;
}

HaffmanEncoder::HaffmanEncoder() : buffer(), char_nodes(256), top_node(nullptr) {}

HaffmanEncoder::HaffmanEncoder(const ByteFrequencies& byte_freq) : buffer(), char_nodes(256), top_node(nullptr) {
	build(byte_freq);
}

void HaffmanEncoder::build(const ByteFrequencies& byte_freq) {
	std::fill(char_nodes.begin(), char_nodes.end(), nullptr);
	buffer.clear();

	buffer.reserve(byte_freq.size() * 2);
	std::vector<Node*> nodes;
	nodes.reserve(byte_freq.size());

	for (const auto& it: byte_freq) {
		auto* node = create_leaf_node(
			NodeContent {
					.freq = it.second,
					.val = it.first
			}
		);
		nodes.push_back(node);
		char_nodes[it.first] = node;
	}

	std::sort(
		nodes.begin(),
		nodes.end(),
		is_node_freq_ge
	);

	while (nodes.size() > 1) {
		top_node = create_parent_node(nodes[nodes.size() - 2], nodes[nodes.size() - 1]);
		nodes.resize(nodes.size() - 2);
		auto it = std::lower_bound(nodes.begin(), nodes.end(), top_node, is_node_freq_ge);
		nodes.insert(it, top_node);
	}
}

Node *HaffmanEncoder::node_by_char(uint8_t c) {
	return char_nodes[c];
}

std::ostream &operator<<(std::ostream &os, const HaffmanEncoder &encoder) {
	os << encoder.char_nodes.size();
	for (auto it = encoder.char_nodes.begin(); it < encoder.char_nodes.end(); it++) {
		if (*it == nullptr) continue;
		os << (it - encoder.char_nodes.begin()) << (*it)->content.freq;
	}
	return os;
}

bool is_node_freq_ge(Node *a, Node *b) {
	return a->content.freq > b->content.freq;
}

ByteFrequencies create_byte_frequencies(std::array<size_t, 256> &char_counts, size_t file_size) {
	ByteFrequencies byte_freq;
	for (int i = 0; i < char_counts.size(); ++i) {
		if (char_counts[i] == 0) continue;
		byte_freq.push_back(std::make_pair(i, static_cast<double>(char_counts[i]) / file_size));
	}
	return byte_freq;
}
