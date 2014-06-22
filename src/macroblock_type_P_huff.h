static Huffman *macroblock_type_P_huff = NULL;
if(!macroblock_type_P_huff){
	macroblock_type_P_huff = new Huffman(
		false,
		(int[]){
			0, 1, 1, 1, 0, 3, 1
		},
		6,
		(unsigned char[]){
			1, 2, 3, 4, 5, 6, 7
		},
		7
	);
	macroblock_type_P_huff->make_codewords();
	macroblock_type_P_huff->make_hash_table();
}	
