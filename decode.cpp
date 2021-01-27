#include <iostream>
#include <fstream>
#include <chrono>
#include "haffman_algorithm.h"

std::pair<size_t, int> decode(HaffmanEncoder& encoder, std::ifstream& in_stream, std::ofstream& out_stream) {
	in_stream >> encoder;
	if (in_stream.fail()) {
		return std::make_pair(0, -1);
	}

	Node* encoder_node = nullptr;
	size_t encoded_file_size = 0;
	bit_set_counted<2048> encoded_bits;
	encoded_bits.count = encoded_bits.size();
	auto* input_buffer = static_cast<std::bitset<2048>*>(&encoded_bits);

	uint8_t output_buffer[256];
	size_t output_count = 0;

	while (true) {
		encoded_bits.pos = 0;
		in_stream.read(reinterpret_cast<char *>(input_buffer), input_buffer->size() / 8);
		if (in_stream.bad()) {
			return std::make_pair(0, -1);
		} else if (in_stream.fail()) {
			encoded_bits.count = in_stream.gcount() * 8;
		}

		while (encoded_bits.pos < encoded_bits.count) {
			if (encoder_node == nullptr) {
				encoder_node = encoder.top();
			}
			encoder_node = encoder.decode(encoder_node, encoded_bits);
			if (encoder.is_leaf(encoder_node)) {
				output_buffer[output_count] = encoder_node->content.val;
				output_count++;
				encoder_node = nullptr;
			}

			if (output_count == sizeof output_buffer) {
				out_stream.write(reinterpret_cast<char*>(&output_buffer), sizeof output_buffer);
				output_count = 0;
				encoded_file_size += sizeof output_buffer;
				if (out_stream.bad() || out_stream.fail()) {
					return std::make_pair(0, -3);
				}
			}
		}

		if (in_stream.eof()) break;
	}

	out_stream.write(reinterpret_cast<char*>(&output_buffer), output_count);
	if (out_stream.bad() || out_stream.fail()) {
		return std::make_pair(0, -3);
	}
	encoded_file_size += output_count;

	return std::make_pair(encoded_file_size, 0);
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

	std::ofstream out_stream(argv[2], std::ios::binary | std::ios::out);
	if (!in_stream.is_open()) {
		std::cout << "Failed to open " << argv[2];
		return -1;
	}

	HaffmanEncoder encoder;
	auto encode_result = decode(encoder, in_stream, out_stream);
	int status = encode_result.second;
	size_t out_file_size = encode_result.first;

	in_stream.close();
	out_stream.close();

	if (status == -1) {
		std::cout << "Fatal error while reading encoder info from " << argv[1] << std::endl;
		return -1;
	} else if (status == -2) {
		std::cout << "Fatal error while reading " << argv[1] << std::endl;
		return -2;
	} else if (status == -3) {
		std::cout << "Fatal error while writing " << argv[2] << std::endl;
		return -3;
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	encoder.print_encoding_info(std::cout);

	std::cout << "Decoding time: " << static_cast<float>(micros) / 1000 << " ms" << std::endl;
	std::cout << "Output file size: " << static_cast<float>(out_file_size) / 1024 << " KBytes" << std::endl;

	return 0;
}