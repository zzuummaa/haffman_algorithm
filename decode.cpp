#include <iostream>
#include <fstream>
#include "haffman_algorithm.h"

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << "Invalid input args";
		return -1;
	}

	std::ifstream in_stream(argv[1], std::ios::binary | std::ios::in);
	if (!in_stream.is_open()) {
		std::cout << "Failed to open " << argv[1];
		return -1;
	}

	HaffmanEncoder encoder;
	in_stream >> encoder;
	if (in_stream.fail()) {
		std::cout << "Fatal error while reading encoder info from " << argv[1] << std::endl;
	}
	in_stream.close();

	encoder.print_encoding_info(std::cout);

	return 0;
}