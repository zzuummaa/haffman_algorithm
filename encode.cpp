#include <fstream>
#include <iostream>
#include <array>
#include <bitset>
#include <vector>
#include <algorithm>
#include <chrono>

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

bool is_node_freq_ge(Node* a, Node* b){
	return a->content.freq > b->content.freq;
}

class HaffmanEncoder {
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

public:
	explicit HaffmanEncoder(std::array<size_t, 256>& char_counts, size_t file_size) : buffer(), char_nodes(256), top_node(nullptr) {
		buffer.reserve(511);
		std::vector<Node*> nodes;
		nodes.reserve(256);

		for (int i = 0; i < char_counts.size(); ++i) {
			if (char_counts[i] == 0) continue;
			auto* node = create_leaf_node(
				NodeContent {
						.freq = static_cast<double>(char_counts[i]) / file_size,
						.val = static_cast<uint8_t>(i)
				}
			);
			nodes.push_back(node);
			char_nodes[i] = node;
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

	template<size_t Nb>
	Node* encode(Node* node, bit_set_counted<Nb>& out) {
		while (node->parent != nullptr && out.count < Nb) {
			out[out.count] = node->parent->left == node;
			out.count++;
			node = node->parent;
		}
		return node->parent;
	}

	Node* node_by_char(uint8_t c) {
		return char_nodes[c];
	}
};

std::pair<size_t, bool> read_char_counts(std::ifstream& in_stream, std::array<size_t, 256>& char_counts) {
	size_t file_size = 0;
	uint8_t buffer[256];
	int count = sizeof(buffer);
	while (true) {
		in_stream.read(reinterpret_cast<char *>(buffer), count);

		if (in_stream.bad()) {
			return std::make_pair(file_size, false);
		} else if (in_stream.fail()) {
			count = in_stream.gcount();
		}

		file_size += count;

		for (int i = 0; i < count; ++i) {
			char_counts[buffer[i]]++;
		}

		if (in_stream.eof()) break;
	}

	return std::make_pair(file_size, true);
}

std::pair<size_t, int> encode(HaffmanEncoder& encoder, std::ifstream& in_stream, std::ofstream& out_stream) {
	Node* encoder_node = nullptr;
	bit_set_counted<2048> encoded_bits;
	size_t encoded_file_size = 0;

	uint8_t input_buffer[256];
	size_t input_count;

	while (true) {
		input_count = sizeof input_buffer;
		in_stream.read(reinterpret_cast<char*>(&input_buffer), sizeof input_buffer);
		if (in_stream.bad()) {
			return std::make_pair(0, -1);
		} else if (in_stream.fail()) {
			input_count = in_stream.gcount();
		}


		while (input_count > 0) {
			if (encoder_node == nullptr) {
				input_count--;
				encoder_node = encoder.encode(encoder.node_by_char(input_buffer[input_count]), encoded_bits);
			} else {
				encoder_node = encoder.encode(encoder_node, encoded_bits);
			}

			if (encoded_bits.is_full()) {
				out_stream.write(reinterpret_cast<char*>(&encoded_bits), sizeof encoded_bits);
				encoded_bits.count = 0;
				if (out_stream.bad() || out_stream.fail()) {
					return std::make_pair(0, -3);
				}
				encoded_file_size += sizeof encoded_bits;
			}
		}

		if (in_stream.eof()) break;
	}

	out_stream.write(reinterpret_cast<char*>(&encoded_bits), encoded_bits.count / 8);
	if (out_stream.bad() || out_stream.fail()) {
		return std::make_pair(0, -3);
	}

	return std::make_pair(encoded_file_size, 0);
}

int print_encoder_info(HaffmanEncoder& encoder, const std::array<size_t, 256>& char_counts) {
	for (auto it = char_counts.begin(); it < char_counts.end(); it++) {
		if (*it == 0) continue;
		uint8_t c = it - char_counts.begin();
		bit_set_counted<256> encoded_char;
		if (encoder.encode(encoder.node_by_char(c), encoded_char) != nullptr) {
			std::cout << "Encode error" << std::endl;
			return -1;
		}

		std::cout << "char_count[" << static_cast<int>(c) << "]\t\tsymb_count=" << *it
				  << ",\tbits=" << encoded_char.to_string().substr(encoded_char.size() - encoded_char.count) << std::endl;
	}
	std::cout << std::endl;
	return 0;
}

int main(int argc, char* argv[]) {
	auto start_time = std::chrono::high_resolution_clock::now();

	if (argc != 3) {
		std::cout << "Invalid input args";
		return -1;
	}

	std::ifstream in_stream(argv[1], std::ios::binary | std::ios::in);
	if (!in_stream.is_open()) {
		std::cout << "Failed to open " << argv[1];
		return -1;
	}

	std::array<size_t, 256> char_counts{};
	auto read_result = read_char_counts(in_stream, char_counts);
	if (!read_result.second) {
		std::cout << "Failed while read from " << argv[1];
		return -1;
	}

	size_t file_size = read_result.first;
	HaffmanEncoder encoder(char_counts, file_size);

	in_stream.clear();
	in_stream.seekg(0);
	if (in_stream.fail()) {
		std::cout << "Fail to seek file to start" << std::endl;
		return -1;
	}

	std::ofstream out_stream(argv[2], std::ios::binary | std::ios::out);
	if (!in_stream.is_open()) {
		std::cout << "Failed to open " << argv[2];
		return -1;
	}

	auto encode_result = encode(encoder, in_stream, out_stream);
	int status = encode_result.second;
	size_t out_file_size = encode_result.first;

	in_stream.close();
	out_stream.close();

	if (status == -1) {
		std::cout << "Fatal error while reading " << argv[1] << std::endl;
	} else if (status == -2) {
		std::cout << "Fatal error encoded sequence not fit to output buffer" << std::endl;
	} else if (status == -3) {
		std::cout << "Fatal error while writing " << argv[2] << std::endl;
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	print_encoder_info(encoder, char_counts);

	std::cout << "Compression time: " << static_cast<float>(micros) / 1000 << " ms" << std::endl;
	std::cout << "Input file size: " << static_cast<float>(file_size) / 1024 << " KBytes" << std::endl;
	std::cout << "Output file size: " << static_cast<float>(out_file_size) / 1024 << " KBytes" << std::endl;

	return status;
}
