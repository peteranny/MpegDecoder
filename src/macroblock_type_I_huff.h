static Huffman *macroblock_type_I_huff = NULL;
if(!macroblock_type_I_huff){
	macroblock_type_I_huff = new Huffman(
		false,
		(int[]){
			0, 1, 1
		},
		2,
		(unsigned char[]){
			1, 2
		},
		2
	);
	macroblock_type_I_huff->make_codewords();
	macroblock_type_I_huff->make_hash_table();
}
