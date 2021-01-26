#include <fstream>
#include <iostream>
#include <array>
#include <bitset>
#include <vector>
#include <algorithm>

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
};

bool is_node_freq_ge(Node* a, Node* b){
	return a->content.freq > b->content.freq;
}

class HaffmanEncoderGraph {
	std::vector<Node> buffer;
	std::vector<Node*> char_nodes;
	Node* top_node;

	Node* create_leaf_node(const NodeContent& content) {
		if (buffer.capacity() == buffer.size()) return nullptr;

		buffer.emplace_back();
		Node* node = &buffer.back();
		node->content = content;

		return node;
	}

	Node* create_parent_node(Node* left_node,  Node* right_node) {
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

	int encode_internal(bit_set_counted<256>& out, Node* node, int depth) {
		if (node->parent == nullptr) return depth;

		out[depth] = node->parent->left == node;
		out.count = depth;
		return encode_internal(out, node->parent, depth + 1);
	}

public:
	explicit HaffmanEncoderGraph(std::array<size_t, 256>& char_counts, size_t file_size) : buffer(), char_nodes(256), top_node(nullptr) {
		buffer.reserve(511);
		std::vector<Node*> nodes(256, nullptr);

		for (int i = 0; i < char_counts.size(); ++i) {
			nodes[i] = create_leaf_node(
				NodeContent {
					.freq = static_cast<double>(char_counts[i]) / file_size,
					.val = static_cast<uint8_t>(i)
				}
			);
			char_nodes[i] = nodes[i];
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

	Node* top() {
		return top_node;
	}

	bit_set_counted<256> encode(uint8_t in) {
		bit_set_counted<256> out{};
		encode_internal(out, char_nodes[in], 0);
		return out;
	}
};

std::pair<size_t, bool> read_char_counts(std::ifstream& in_stream, std::array<size_t, 256>& char_counts) {
	size_t file_size = 0;
	uint8_t buffer[256];
	int count = sizeof(buffer);
	while (true) {
		in_stream.read(reinterpret_cast<char *>(buffer), count);

		if (!in_stream) {
			if (in_stream.eof()) {
				break;
			} else if (in_stream.bad()) {
				return std::make_pair(file_size, false);
			} else {
				count = in_stream.gcount();
				continue;
			}
		}

		file_size += count;

		for (int i = 0; i < count; ++i) {
			char_counts[buffer[i]]++;
		}
	}

	return std::make_pair(file_size, true);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << "Invalid input args";
		return -1;
	}

	std::ifstream in_stream(argv[1], std::ios::binary);
	if (!in_stream.is_open()) {
		std::cout << "Failed to open " << argv[1];
		return -1;
	}

	std::array<size_t, 256> char_counts{};
	auto result = read_char_counts(in_stream, char_counts);
	if (!result.second) {
		std::cout << "Failed while read from " << argv[1];
		return -1;
	}

	size_t file_size = result.first;
	HaffmanEncoderGraph encoder(char_counts, file_size);

	for (auto it = char_counts.begin(); it < char_counts.end(); it++) {
		uint8_t c = it - char_counts.begin();
		std::cout << "char_count[" << static_cast<int>(c) << "]\t= " << *it << encoder.encode(c).to_ullong() << std::endl;
	}
	std::cout << std::endl;

	return 0;
}
