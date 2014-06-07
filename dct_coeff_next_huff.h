dct_coeff_next_huff = new Huffman(
	true,
	(int[]){
	0, 
	},
	0,
	(unsigned char[]){
	},
	0
);
dct_coeff_next_huff->make_codewords();
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0x00, 0x00), "10"); // end_of_block
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 1), "11"); // only difference from dct_coeff_next
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 1), "011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 2), "0100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(2, 1), "0101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 3), "00101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(3, 1), "00111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(4, 1), "00110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 2), "000110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(5, 1), "000111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(6, 1), "000101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(7, 1), "000100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 4), "0000110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(2, 2), "0000100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(8, 1), "0000111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(9, 1), "0000101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0xFF, 0xFF), "000001"); // escape
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 5), "00100110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 6), "00100001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 3), "00100101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(3, 2), "00100100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(10, 1), "00100111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(11, 1), "00100011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(12, 1), "00100010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(13, 1), "00100000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 7), "0000001010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 4), "0000001100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(2, 3), "0000001011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(4, 2), "0000001111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(5, 2), "0000001001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(14, 1), "0000001110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(15, 1), "0000001101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(16, 1), "0000001000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 8), "000000011101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 9), "000000011000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 10), "000000010011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 11), "000000010000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 5), "000000011011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(2, 4), "000000010100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(3, 3), "000000011100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(4, 3), "000000010010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(6, 2), "000000011110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(7, 2), "000000010101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(8, 2), "000000010001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(17, 1), "000000011111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(18, 1), "000000011010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(19, 1), "000000011001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(20, 1), "000000010111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(21, 1), "000000010110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 12), "0000000011010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 13), "0000000011001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 14), "0000000011000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 15), "0000000010111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 6), "0000000010110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 7), "0000000010101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(2, 5), "0000000010100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(3, 4), "0000000010011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(5, 3), "0000000010010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(9, 2), "0000000010001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(10, 2), "0000000010000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(22, 1), "0000000011111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(23, 1), "0000000011110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(24, 1), "0000000011101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(25, 1), "0000000011100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(26, 1), "0000000011011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 16), "00000000011111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 17), "00000000011110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 18), "00000000011101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 19), "00000000011100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 20), "00000000011011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 21), "00000000011010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 22), "00000000011001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 23), "00000000011000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 24), "00000000010111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 25), "00000000010110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 26), "00000000010101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 27), "00000000010100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 28), "00000000010011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 29), "00000000010010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 30), "00000000010001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 31), "00000000010000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 32), "000000000011000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 33), "000000000010111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 34), "000000000010110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 35), "000000000010101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 36), "000000000010100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 37), "000000000010011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 38), "000000000010010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 39), "000000000010001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(0, 40), "000000000010000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 8), "000000000011111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 9), "000000000011110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 10), "000000000011101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 11), "000000000011100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 12), "000000000011011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 13), "000000000011010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 14), "000000000011001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 15), "0000000000010011");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 16), "0000000000010010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 17), "0000000000010001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(1, 18), "0000000000010000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(6, 3), "0000000000010100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(11, 2), "0000000000011010");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(12, 2), "0000000000011001");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(13, 2), "0000000000011000");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(14, 2), "0000000000010111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(15, 2), "0000000000010110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(16, 2), "0000000000010101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(27, 1), "0000000000011111");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(28, 1), "0000000000011110");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(29, 1), "0000000000011101");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(30, 1), "0000000000011100");
dct_coeff_next_huff->set_codeword(dct_coeff_symbol(31, 1), "0000000000011011");
dct_coeff_next_huff->make_hash_table();
