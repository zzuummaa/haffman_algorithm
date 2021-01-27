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
	uint16_t count = encoder.char_nodes.size() - std::count(encoder.char_nodes.begin(), encoder.char_nodes.end(), nullptr);
	os.write(reinterpret_cast<char*>(&count), sizeof(count));
	for (auto it = encoder.char_nodes.begin(); it < encoder.char_nodes.end(); it++) {
		if (*it == nullptr) continue;

		uint8_t c = static_cast<uint8_t>(it - encoder.char_nodes.begin());
		os.write(reinterpret_cast<char*>(&c), sizeof(c));

		double freq = (*it)->content.freq;
		os.write(reinterpret_cast<char*>(&freq), sizeof(freq));
	}
	return os;
}

std::istream &operator>>(std::istream &is, HaffmanEncoder &encoder) {
	uint16_t count;
	is.read(reinterpret_cast<char*>(&count), sizeof(count));
	if (is.fail() || count > 256) {
		is.setstate(is.rdstate() | std::ios::failbit);
		return is;
	}

	ByteFrequencies byte_freq(count);
	double sum_freq = 0;
	for (int i = 0; i < count; ++i) {
		is.read(reinterpret_cast<char*>(&byte_freq[i].first), sizeof(byte_freq[i].first));
		if (is.fail()) {
			return is;
		}
		is.read(reinterpret_cast<char*>(&byte_freq[i].second), sizeof(byte_freq[i].second));
		if (is.fail()) {
			return is;
		}

		if (i > 0) {
			if (byte_freq[i - 1].first >= byte_freq[i].first) {
				is.setstate(is.rdstate() | std::ios::failbit);
				return is;
			}
		}
		sum_freq += byte_freq[i].second;
	}

	if (std::abs(1.0 - sum_freq) > 0.1) {
		is.setstate(is.rdstate() | std::ios::failbit);
		return is;
	}

	encoder.build(byte_freq);

	return is;
}

int HaffmanEncoder::print_encoding_info(std::ostream &os) {
	for (auto it = char_nodes.begin(); it < char_nodes.end(); it++) {
		if (*it == nullptr) continue;
		uint8_t c = it - char_nodes.begin();
		bit_set_counted<256> encoded_char;
		if (encode(node_by_char(c), encoded_char) != nullptr) {
			os << "Encode error" << std::endl;
			return -1;
		}

		os << "char_count[" << static_cast<int>(c) << "]\t\tsymb_freq=" << (*it)->content.freq
				  << ",\tbits=" << encoded_char.to_string().substr(encoded_char.size() - encoded_char.count) << std::endl;
	}
	os << std::endl;
	return 0;
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
