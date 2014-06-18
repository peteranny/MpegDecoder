#include <cstring>
#include "libbit.h"

#ifndef HUFFMAN
#define HUFFMAN
class Huffman{
	private:
		int *nCodesOfLen;
		int maxLen;

		unsigned char *symbol; // symbol index -> symbol
		int *index; // symbol -> index
		int nSymbols;

		int *codeword; // symbol index -> symbol codeword
		int *codelen; // symbol index -> symbol codelen
		bool isIncr;

		int *hash; // codeword -> symbol index
		int nCodewords;
	public:
		Huffman(bool isIncr, int nCodesOfLen[], int maxLen, unsigned char symbol[], int nSymbols){
			this->isIncr = isIncr;

			this->maxLen = maxLen;
			this->nCodesOfLen = new int[maxLen + 1];
			memcpy(this->nCodesOfLen, nCodesOfLen, sizeof(int)*(maxLen + 1));

			this->nSymbols = nSymbols;
			this->symbol = new unsigned char[1 + nSymbols + 1]; // both boundary symbols
			this->index = new int[256];
			memcpy(&this->symbol[1], symbol, sizeof(unsigned char)*(nSymbols));

			this->codelen = new int[1 + nSymbols + 1];
			this->codeword = new int[1 + nSymbols + 1];
			
			this->nCodewords = pow(2, maxLen);
			this->hash = new int[nCodewords];

			return;
		}
		void print_codewords(){
			for(int i = 0; i <= nSymbols + 1; i++){
				if(codeword[i] == -1 || codeword[i] == nCodewords){
					fprintf(stderr, "Huffman.print_codewords(): i=%2d,       codeword=%4d, symbol=%s\n", i, codeword[i], (i == 0)? "START": "END");
				}
				else{
					fprintf(stderr, "Huffman.print_codewords(): i=%2d, l=%2d, codeword=%4d=", i, codelen[i], codeword[i]);
					fprintb(stderr, codeword[i], maxLen);
					fprintf(stderr, "(");
					fprintb(stderr, codeword[i] >> (maxLen - codelen[i]), codelen[i]);
					fprintf(stderr, "), symbol=%02X(%d)\n", symbol[i], symbol[i]);
				}
			}
		}
		void print_codeword(unsigned char symbol){
			int i = get_index(symbol);
			fprintb(stderr, codeword[i] >> (maxLen - codelen[i]), codelen[i]);
			return;
		}
		void make_codewords(){
			int i = 0; // symbol index

			// boundary symbol used for hash table
			this->codeword[i] = isIncr? -1: nCodewords;
			//fprintf(stderr, "Huffman.make_codewords(): i=%2d,       codeword=%4d\n", i, this->codeword[0]);
			i++;
			
			int codeword = isIncr? 0: 1 << (maxLen - 1); // integer version
			for(int l = 1; l <= maxLen; l++){
				for(int j = 0; j < nCodesOfLen[l]; j++){
					if(!isIncr && i != 1) codeword -= 1 << (maxLen - l);
					//fprintf(stderr, "Huffman.make_codewords(): i=%2d, l=%2d, codeword=%4d=", i, l, codeword);
					//fprintb(stderr, codeword, maxLen);
					//fprintf(stderr, "(");
					//fprintb(stderr, codeword >> (maxLen - l), l);
					//fprintf(stderr, "), symbol=%02X(%d)\n", symbol[i], symbol[i]);
					this->codeword[i] = codeword;
					this->codelen[i] = l;
					this->index[(int)symbol[i]] = i;
					i++;
					if(isIncr) codeword += 1 << (maxLen - l);
				}
			}
			if(i > nSymbols + 1) EXIT("Huffman.make_codewords: error() // i");

			// boundary symbol used for hash table
			this->codeword[i] = isIncr? nCodewords: -1;
			//fprintf(stderr, "Huffman.make_codewords(): i=%2d,       codeword=%4d\n", i, this->codeword[i]);
			return;
		}
	public:
		int get_maxLen(){
			return maxLen;
		}
		int get_index(unsigned char symbol){
			// check if duplicate symbols
			for(int i = 1; i <= nSymbols; i++) for(int j = 1; j < i; j++){
				if(this->symbol[i] == this->symbol[j]) EXIT("Huffman.get_index(): error");
			}
			// check if symbol exists
			for(int i = 1; i <= nSymbols; i++){
				if(this->symbol[i] == symbol) return index[(int)symbol];
			}
			return -1; // not exist
		}
		int get_codelen(unsigned char symbol){
			int i = get_index((int)symbol);
			return codelen[i];
		}
		void set_codeword(unsigned char symbol, const char *codeword_str){
			//fprintf(stderr, "Huffman.set_codeword(): symbol=%02X, codeword_str=%s\n", symbol, codeword_str);
			// parse codeword_str
			int codelen = strlen(codeword_str);
			if(codelen > maxLen){
				// extend if longer codelen
				for(int i = 1; i <= nSymbols; i++){
					this->codeword[i] <<= codelen - maxLen;
				}
				maxLen = codelen;
				nCodewords = 1 << maxLen;
				this->codeword[isIncr? nSymbols + 1: 0] = nCodewords;
				delete this->hash;
				this->hash = new int[nCodewords];
				// let alone nCodesOfLen
			}
			int codeword = parsebit(codeword_str) << (maxLen - codelen);
			// set codeword
			int i = get_index(symbol);
			if(i >= 0){
				this->codelen[i] = codelen;
				this->codeword[i] = codeword;
			}
			else{
				unsigned char old_symbol[1 + nSymbols + 1];
				int old_codeword[1 + nSymbols + 1];
				int old_codelen[1 + nSymbols + 1];
				memcpy(old_symbol, this->symbol, sizeof(unsigned char)*(1 + nSymbols + 1));
				memcpy(old_codeword, this->codeword, sizeof(int)*(1 + nSymbols + 1));
				memcpy(old_codelen, this->codelen, sizeof(int)*(1 + nSymbols + 1));
				delete this->symbol;
				delete this->codeword;
				delete this->codelen;

				this->symbol = new unsigned char[1 + (nSymbols + 1) + 1];
				this->codeword = new int[1 + (nSymbols + 1) + 1];
				this->codelen = new int[1 + (nSymbols + 1) + 1];
				memcpy(this->symbol, old_symbol, sizeof(unsigned char)*(1 + nSymbols));
				memcpy(this->codeword, old_codeword, sizeof(int)*(1 + nSymbols));
				memcpy(this->codelen, old_codelen, sizeof(int)*(1 + nSymbols));
				this->symbol[1 + nSymbols] = symbol;
				this->codeword[1 + nSymbols] = codeword;
				this->codelen[1 + nSymbols] = codelen;
				this->index[symbol] = 1 + nSymbols;
				memcpy(&this->symbol[1 + nSymbols + 1], &old_symbol[nSymbols + 1], sizeof(unsigned char));
				memcpy(&this->codeword[1 + nSymbols + 1], &old_codeword[nSymbols + 1], sizeof(int));
				memcpy(&this->codelen[1 + nSymbols + 1], &old_codelen[nSymbols + 1], sizeof(int));
				nSymbols++;
			}
			//fprintf(stderr, "Huffman.set_codeword():   i=%2d, l=%2d, codeword=%4d=", i, codelen, this->codeword[i]);
			//fprintb(stderr, this->codeword[i], maxLen);
			//fprintf(stderr, "(");
			//fprintb(stderr, this->codeword[i] >> (maxLen - codelen), codelen);
			//fprintf(stderr, "), symbol=%02X(%d)\n", this->symbol[i], this->symbol[i]);
			
			// sort codewords
			for(int j = 0; j <= nSymbols + 1; j++) for(int k = 0; k < j; k++){
				if(this->codeword[k] == this->codeword[j]) EXIT("Huffman.set_codeword(): error.");
				if((isIncr && this->codeword[k] > this->codeword[j]) || (!isIncr && this->codeword[k] < this->codeword[j])){
					int tmp;
					unsigned char tmp2;

					tmp2 = this->symbol[j];
					this->symbol[j] = this->symbol[k];
					this->symbol[k] = tmp2;

					this->index[(int)this->symbol[j]] = j;
					this->index[(int)this->symbol[k]] = k;

					tmp = this->codeword[j];
					this->codeword[j] = this->codeword[k];
					this->codeword[k] = tmp;

					tmp = this->codelen[j];
					this->codelen[j] = this->codelen[k];
					this->codelen[k] = tmp;
				}
			}
			return;
		}
		void make_hash_table(){
			//print_codewords();
			// produce huffman hash function
			int i = isIncr? 0: nSymbols + 1; // hashed symbol index
			for(int codeword = 0; codeword < nCodewords; codeword++){
				if(codeword >= this->codeword[isIncr? i + 1: i - 1]) isIncr? i++: i--;
				this->hash[codeword] = i;
				if(codeword == this->codeword[i]){
					//fprintf(stderr, "Huffman.make_hash_table(): i=%2d, symbol[%2d]=%3d, codeword[%d]=%d <= %d < codeword[%d]=%d\n", i, i, symbol[i], i, this->codeword[i], codeword, isIncr? i + 1: i - 1, this->codeword[isIncr? i + 1: i - 1]);
				}
				else if(codeword == this->codeword[i] + 1){
					//fprintf(stderr, "Huffman.make_hash_table(): ...\n");
				}
			}
			//fprintf(stderr, "\n");
			return;
		}
		unsigned char decode(int codeword){
			//fprintf(stderr, "Huffman.decode(): codeword=%d=", codeword);
			//fprintb(stderr, codeword, maxLen);
			int index = hash[codeword];
			if(index == 0 || index == nSymbols + 1) EXIT("Huffman.decode(): error");
			unsigned char symbol = this->symbol[index];
			//fprintf(stderr, ", symbol=%02X(%d)\n", symbol, symbol);
			return symbol;
		}
};
#endif

