macroblock_address_increment_huff = new Huffman(
	false,
	(int[]){
	0, 1, 0, 2, 2, 2, 0, 2, 6, 0, 6, 14
	},
	11,
	(unsigned char[]){
	1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, 32, 33,
	0x00, // macroblock_stuffing
	0xFF // macroblock_escape 
	},
	35
);
macroblock_address_increment_huff->make_codewords();
macroblock_address_increment_huff->set_codeword(0x00, "00000001111");
macroblock_address_increment_huff->set_codeword(0xFF, "00000001000");
macroblock_address_increment_huff->make_hash_table();

