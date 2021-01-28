#include <iostream>
#include <fstream>
#include <chrono>
#include "haffman_algorithm.h"

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

template<std::size_t Nb>
void reverse_bits(std::bitset<Nb> &b, size_t begin_i, size_t end_i) {
	for(size_t i = 0; i < (end_i - begin_i) / 2; ++i) {
		bool t = b[i + begin_i];
		b[i + begin_i] = b[end_i - 1 - i];
		b[end_i - 1 - i] = t;
	}
}

std::pair<size_t, int> encode(HaffmanEncoder& encoder, std::ifstream& in_stream, std::ofstream& out_stream) {
	Node* encoder_node = nullptr;
	size_t cur_byte_pos = 0;
	bit_set_counted<2048> encoded_bits;
	auto* out_buffer = static_cast<std::bitset<2048>*>(&encoded_bits);
	size_t encoded_file_size = 0;

	uint8_t input_buffer[256];
	size_t input_count;

	encoded_file_size += serialize(out_stream, encoder);
	if (out_stream.fail()) {
		return std::make_pair(0, -3);
	}

	while (true) {
		input_count = sizeof input_buffer;
		in_stream.read(reinterpret_cast<char*>(&input_buffer), sizeof input_buffer);
		if (in_stream.bad()) {
			return std::make_pair(0, -1);
		} else if (in_stream.fail()) {
			input_count = in_stream.gcount();
		}

		size_t input_buffer_pos = 0;
		while (input_buffer_pos < input_count) {
			if (encoder_node == nullptr) {
				reverse_bits(encoded_bits, cur_byte_pos, encoded_bits.count);
				cur_byte_pos = encoded_bits.count;
				encoder_node = encoder.encode(encoder.node_by_char(input_buffer[input_buffer_pos]), encoded_bits);
				input_buffer_pos++;
			} else {
				encoder_node = encoder.encode(encoder_node, encoded_bits);
			}

			if (encoded_bits.is_full()) {
				size_t write_size = cur_byte_pos / 8;
				out_stream.write(reinterpret_cast<const char *>(out_buffer), write_size);
				if (out_stream.fail()) {
					return std::make_pair(0, -3);
				}
				copy_bits(encoded_bits, write_size * 8, encoded_bits.count, 0);
				cur_byte_pos = cur_byte_pos - write_size * 8;
				encoded_bits.count = encoded_bits.count - write_size * 8;
				encoded_file_size += write_size;
			}
		}

		if (in_stream.eof()) break;
	}

	if (encoder_node != nullptr) {
		if (encoder.encode(encoder_node, encoded_bits) != nullptr) return std::make_pair(0, -1);
	}
//	std::cout << "last char: ";
//	for (int i = cur_byte_pos; i < encoded_bits.count; ++i) {
//		std::cout << encoded_bits.test(i);
//	}
//	std::cout << std::endl;
	reverse_bits(encoded_bits, cur_byte_pos, encoded_bits.count);
	uint8_t padding_bits_count = encoded_bits.count % 8 > 0 ? 8 - encoded_bits.count % 8 : 0;
	size_t remaining_bytes_count = encoded_bits.count / 8 + (padding_bits_count > 0 ? 1 : 0);
	out_stream.write(reinterpret_cast<const char *>(out_buffer), remaining_bytes_count);
	if (out_stream.fail()) {
		return std::make_pair(0, -3);
	}
	encoded_file_size += remaining_bytes_count;

	out_stream.write(reinterpret_cast<char*>(&padding_bits_count), sizeof padding_bits_count);
	if (out_stream.fail()) {
		return std::make_pair(0, -3);
	}
	encoded_file_size += sizeof padding_bits_count;

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

	std::array<size_t, 256> char_counts{};
	auto read_result = read_char_counts(in_stream, char_counts);
	if (!read_result.second) {
		std::cout << "Failed while read from " << argv[1];
		return -1;
	}

	size_t file_size = read_result.first;
	HaffmanEncoder encoder(create_byte_frequencies(char_counts, file_size));

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

	auto end_time = std::chrono::high_resolution_clock::now();
	auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	if (status == -1) {
		std::cout << "Fatal error while reading " << argv[1] << std::endl;
		return -1;
	} else if (status == -2) {
		std::cout << "Fatal error encoded sequence not fit to output buffer" << std::endl;
		return -2;
	} else if (status == -3) {
		std::cout << "Fatal error while writing " << argv[2] << std::endl;
		return -3;
	}

	encoder.print_encoding_info(std::cout);

	std::cout << "Encoding time: " << static_cast<float>(micros) / 1000 << " ms" << std::endl;
	std::cout << "Input file size: " << static_cast<float>(file_size) / 1024 << " KBytes" << std::endl;
	std::cout << "Output file size: " << static_cast<float>(out_file_size) / 1024 << " KBytes" << std::endl;

	return status;
}
