static Huffman *macroblock_type_B_huff = NULL;
if(!macroblock_type_B_huff){
	macroblock_type_B_huff = new Huffman(
		false,
		(int[]){
			0, 0, 1, 2, 2, 2, 3
		},
		6,
		(unsigned char[]){
			1, 4, 3, 6, 5, 7, 8, 9, 10, 11
		},
		10
	);
	macroblock_type_B_huff->make_codewords();
	macroblock_type_B_huff->set_codeword(2, "11");
	macroblock_type_B_huff->make_hash_table();
}
