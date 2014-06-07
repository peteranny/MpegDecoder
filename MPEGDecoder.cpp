#include <cstdio>
#include <cmath>
#include <vector>
#include <map>
#include <cstdlib>
/*
#include <windows.h>
#include <time.h>
#include <iostream> 
#include <process.h>
*/
#define SEQUENCE_HEADER_CODE 0x1B3
#define GROUP_START_CODE 0x1B8
#define PICTURE_START_CODE 0x100
#define EXTENSION_START_CODE 0x1B5
#define USER_DATA_START_CODE 0x1B2
#define I_FRAME 1
#define P_FRAME 2
#define B_FRAME 3
#define D_FRAME 4
using namespace std;

//#include <windows.h>



class MPEGDecoder{
public:
	
	class HuffmanDecoding{
	public:
		HuffmanDecoding(){
			this->maxHuffmanTreeDepth = 8;
		}
		
		/* Generating a Huffman table for the decoding algorithm.
		 * lengthArray: The array of code lengths from the file
		 * symbolArray: The array of symbols from the file
		 * isDC: Whether the table belongs to DC. If false, the table belongs to AC
		 */
		void add(int newSymbol, int newCode, int newLength){
			TableData data;
			data.symbol = newSymbol;
			data.codeword = newCode;
			data.codeLength = newLength;
			this->huffmanTable.push_back(data);
		}
		
		void initialize(){
			constructLookUpTable(this->huffmanTable, 0);
		}
		
		/* Implementation of the Huffman decoding.
		 * file: The pointer of the file
		 * startBit: Which bit is read first in the first byte
		 * RRRR: The value of RRRR, the first four bits in an AC symbol
		 * Return value: Final decoded value
		 */
		int decode(MPEGDecoder &mpeg){
			int currentSuperTreeNode = 0;
			int symbol;
	
			// Decode the codeword
			while(1){
				int bufferLength = this->superTreeTable[currentSuperTreeNode].depth + 1;
				int superTreeAddress = this->superTreeTable[currentSuperTreeNode].address;
				
				int index = mpeg.nextBits(bufferLength, true);
				
				LookUpTableData data = this->lookUpTables[currentSuperTreeNode][index];
				
				if(data.sign == 0){
					int dataAddress = superTreeAddress + data.offset;
					symbol = this->mainMemory[dataAddress];
					mpeg.nextBits(data.codeLength);
					break;
				}
				else{
					currentSuperTreeNode = data.offset;
					mpeg.nextBits(bufferLength);
				}
			}
			return symbol;
		}
			
	private:
		//======== Table initialization ========
	
		int maxHuffmanTreeDepth;
		vector<int> mainMemory; // Main memory: Symbols stored here to be looked up
		
		class SuperTreeTableData{
		public:
			int depth;
			int address;
		};
		vector<SuperTreeTableData> superTreeTable; // S-tree table: The depth and the address of every sub-tree
	
		class LookUpTableData{
		public:
			char sign; 
			int codeLength;
			int offset;
		};
		vector< vector<LookUpTableData> > lookUpTables; // Look-up tables: Offering look-up of symbols
	
		class TableData{
		public:
			int codeLength;
			int symbol;
			int codeword;
		};
		vector<TableData> huffmanTable; // Huffman table: The given huffman code table
	
		//======== Table content construction ========	
		
		/* Getting some bits in the codeword.
		 * codeword: The given codeword
		 * codeLength: The length of the codeword
		 * startBit: What bit is read first
		 * getLength: How many bits to be read
		 * Return value: The demanded bits
		 */
		int getBitRange(int codeword, int codeLength, int startBit, int getLength){
			int result = 0;
			for(int i = 0; i < getLength; i ++){
				int bit = (codeword >> (codeLength - 1 - startBit - i)) & 1;
				result = (result << 1) | bit;
			}
			return result;
		}
		
		/* Contruct all decoding tables (LUT, S-tree) recursively from the given huffman table.
		 * codeTable: The original or sub huffman table to construct decoding tables.
		 * codeStartIndex: The start index of a codeword for each decoding table.
		 * Return value: The S-tree index used for building the connection between two LUTs.
		 */
		int constructLookUpTable(vector<TableData> codeTable, int codeStartIndex){
			int treeDepth = this->countMaxCodeLength(codeTable, codeStartIndex);
		
			// Construct LUT
			vector<LookUpTableData> lookUpTable(1 << treeDepth);
			this->lookUpTables.push_back(lookUpTable);
			
			// Construct S-tree
			int treeIndex = this->superTreeTable.size();
			SuperTreeTableData treeData;
			treeData.depth = treeDepth - 1;
			treeData.address = this->mainMemory.size();
			this->superTreeTable.push_back(treeData);
			
			int memoryOffset = 0;
			map<int, vector<TableData> > newLookUpTables;
			
			for(vector<TableData>::iterator data = codeTable.begin(); data != codeTable.end(); data ++){
				int residualCodeLength = data->codeLength - codeStartIndex;
				int dataCodeLength =  (residualCodeLength <= this->maxHuffmanTreeDepth)? residualCodeLength: this->maxHuffmanTreeDepth;
				
				int index = getBitRange(data->codeword, data->codeLength, codeStartIndex, dataCodeLength);
				for(int i = dataCodeLength; i < treeDepth; i ++){
					index <<= 1;
				}
				
				// If a codeword is short enough to be looked up in this LUT, then store it to the main memory
				if(residualCodeLength <= this->maxHuffmanTreeDepth){
					this->mainMemory.push_back(data->symbol);
						
					int fillingRange = 1 << (treeDepth - dataCodeLength);
					for(int i = 0; i < fillingRange; i ++){
						this->lookUpTables[treeIndex][index + i].sign = 0;
						this->lookUpTables[treeIndex][index + i].codeLength = dataCodeLength;
						this->lookUpTables[treeIndex][index + i].offset = memoryOffset;
					}
				
					memoryOffset ++;
				}
				else{
					// Collect the longer codewords that need another LUT to be looked up
					if(newLookUpTables.find(index) == newLookUpTables.end()){
						vector<TableData> newTable;
						newLookUpTables[index] = newTable;
					}
					newLookUpTables[index].push_back(*data);
				}
			}
			
			// Recursively construct another tables
			for(map<int, vector<TableData> >::iterator iter = newLookUpTables.begin(); iter != newLookUpTables.end(); iter++){
				int index = iter->first;
				vector<TableData> subCodeTable = iter->second;
				this->lookUpTables[treeIndex][index].sign = 1;
				int pointedTreeIndex = this->constructLookUpTable(subCodeTable, codeStartIndex + this->maxHuffmanTreeDepth);
				this->lookUpTables[treeIndex][index].offset = pointedTreeIndex;
			}
			
			return treeIndex;
		}
	
		/* Compute the maximum possible codeword length i.e. the depth of a tree.
		 * table: A given huffman table.
		 * codeStartIndex: The start index of a codeword for each decoding table.
		 * Return value: The maximum value.
		 */
		int countMaxCodeLength(vector<TableData> table, int codeStartIndex){
			int maxLength = 0;
			for(vector<TableData>::iterator data = table.begin(); data != table.end(); data ++){
				int length = data->codeLength - codeStartIndex;
				if(maxLength < length){
					maxLength = length;
				}
				if(maxLength >= this->maxHuffmanTreeDepth){
					return this->maxHuffmanTreeDepth;
				}
			}
			return maxLength;
		}
	
	};
	
	class Block{
	public:
		int table[8][8];
		double cosPI16[8];
		Block(){
			for(int i = 0; i < 8; i ++){
				this->cosPI16[i] = cos(i * 3.14159265358979 / 16);
			}
		}
		
		int read(int row, int column){
			return this->table[row][column];
		}
		
		void write(int row, int column, int value){
			this->table[row][column] = value;
		}
		
		void zigzag(int array[]){
			this->table[0][0] = array[0];
			this->table[0][1] = array[1];
			this->table[1][0] = array[2];
			this->table[2][0] = array[3];
			this->table[1][1] = array[4];
			this->table[0][2] = array[5];
			this->table[0][3] = array[6];
			this->table[1][2] = array[7];
			this->table[2][1] = array[8];
			this->table[3][0] = array[9];
			this->table[4][0] = array[10];
			this->table[3][1] = array[11];
			this->table[2][2] = array[12];
			this->table[1][3] = array[13];
			this->table[0][4] = array[14];
			this->table[0][5] = array[15];
			this->table[1][4] = array[16];
			this->table[2][3] = array[17];
			this->table[3][2] = array[18];
			this->table[4][1] = array[19];
			this->table[5][0] = array[20];
			this->table[6][0] = array[21];
			this->table[5][1] = array[22];
			this->table[4][2] = array[23];
			this->table[3][3] = array[24];
			this->table[2][4] = array[25];
			this->table[1][5] = array[26];
			this->table[0][6] = array[27];
			this->table[0][7] = array[28];
			this->table[1][6] = array[29];
			this->table[2][5] = array[30];
			this->table[3][4] = array[31];
			this->table[4][3] = array[32];
			this->table[5][2] = array[33];
			this->table[6][1] = array[34];
			this->table[7][0] = array[35];
			this->table[7][1] = array[36];
			this->table[6][2] = array[37];
			this->table[5][3] = array[38];
			this->table[4][4] = array[39];
			this->table[3][5] = array[40];
			this->table[2][6] = array[41];
			this->table[1][7] = array[42];
			this->table[2][7] = array[43];
			this->table[3][6] = array[44];
			this->table[4][5] = array[45];
			this->table[5][4] = array[46];
			this->table[6][3] = array[47];
			this->table[7][2] = array[48];
			this->table[7][3] = array[49];
			this->table[6][4] = array[50];
			this->table[5][5] = array[51];
			this->table[4][6] = array[52];
			this->table[3][7] = array[53];
			this->table[4][7] = array[54];
			this->table[5][6] = array[55];
			this->table[6][5] = array[56];
			this->table[7][4] = array[57];
			this->table[7][5] = array[58];
			this->table[6][6] = array[59];
			this->table[5][7] = array[60];
			this->table[6][7] = array[61];
			this->table[7][6] = array[62];
			this->table[7][7] = array[63];
		}
		
		void IDCT1D(double target[][8], int index, bool isRow){
			double x[8], y[8];
			for(int i = 0; i < 8; i ++){
				x[i] = (isRow)? target[index][i]: target[i][index];
			}
			
			double t0 = x[0] * this->cosPI16[4];
			double t1 = x[2] * this->cosPI16[2];
			double t2 = x[2] * this->cosPI16[6];
			double t3 = x[4] * this->cosPI16[4];
			double t4 = x[6] * this->cosPI16[6];
			double t5 = x[6] * this->cosPI16[2];
			
			double a0 = t0 + t1 + t3 + t4;
			double a1 = t0 + t2 - t3 - t5;
			double a2 = t0 - t2 - t3 + t5;
			double a3 = t0 - t1 + t3 - t4;
			
			double b0 = x[1] * this->cosPI16[1] + x[3] * this->cosPI16[3] + x[5] * this->cosPI16[5] + x[7] * this->cosPI16[7];
			double b1 = x[1] * this->cosPI16[3] - x[3] * this->cosPI16[7] - x[5] * this->cosPI16[1] - x[7] * this->cosPI16[5];
			double b2 = x[1] * this->cosPI16[5] - x[3] * this->cosPI16[1] + x[5] * this->cosPI16[7] + x[7] * this->cosPI16[3];
			double b3 = x[1] * this->cosPI16[7] - x[3] * this->cosPI16[5] + x[5] * this->cosPI16[3] - x[7] * this->cosPI16[1];
			
			y[0] = (a0 + b0) * 0.5;
			y[7] = (a0 - b0) * 0.5;
			y[1] = (a1 + b1) * 0.5;
			y[6] = (a1 - b1) * 0.5;
			y[2] = (a2 + b2) * 0.5;
			y[5] = (a2 - b2) * 0.5;
			y[3] = (a3 + b3) * 0.5;
			y[4] = (a3 - b3) * 0.5;
			
			for(int i = 0; i < 8; i ++){
				if(isRow){
					target[index][i] = y[i];
				}
				else{
					target[i][index] = y[i];
				}
			}
		}
		
		void IDCT(){
			
			double doubleTable[8][8];
			for(int i = 0; i < 8; i ++){
				for(int j = 0; j < 8; j ++){
					doubleTable[i][j] = (double)this->table[i][j];
				}
			}
			for(int i = 0; i < 8; i ++){
				IDCT1D(doubleTable, i, true);
			}
			for(int i = 0; i < 8; i ++){
				IDCT1D(doubleTable, i, false);
			}
			for(int i = 0; i < 8; i ++){
				for(int j = 0; j < 8; j ++){
					this->table[i][j] = (int)round(doubleTable[i][j]);
				}
			}
			
		}
		
		void positiveCells(){
			for(int i = 0; i < 8; i ++){
				for(int j = 0; j < 8; j ++){
					if(this->table[i][j] < 0){
						this->table[i][j] = 0;
					}
					if(this->table[i][j] > 255){
						this->table[i][j] = 255;
					}
				}
			}
		}
		
		void print(){
			for(int i = 0; i < 8; i ++){
				for(int j = 0; j < 8; j ++){
					printf("%d\t", this->table[i][j]);
				}
				printf("\n");
			}
			printf("\n");
		}
    };
    
    class Pixel{
	public:
		int Y;
		int Cb;
		int Cr;
		int R;
		int G;
		int B;
		void RGBTrans(){
			boundary(Y);
			boundary(Cb);
			boundary(Cr);
			R = (int)round(1.16438356164 * (Y - 16.0) + 1.59602678571 * (Cr - 128.0));
			G = (int)round(1.16438356164 * (Y - 16.0) - 0.39176229009 * (Cb - 128.0) - 0.81296764723 * (Cr - 128.0));
			B = (int)round(1.16438356164 * (Y - 16.0) + 2.01723214286 * (Cb - 128.0));
			boundary(R);
			boundary(G);
			boundary(B);
		}
		void boundary(int &color){
			if(color < 0){
				color = 0;
			}
			if(color > 255){
				color = 255;
			}
		}
	};

	HuffmanDecoding macroblockAddressIncrementTable;
	HuffmanDecoding macroblockTypeITable;
	HuffmanDecoding macroblockTypePTable;
	HuffmanDecoding macroblockTypeBTable;
	HuffmanDecoding motionVectorTable;
	HuffmanDecoding macroblockPatternTable;
	HuffmanDecoding dctDcSizeLuminanceTable;
	HuffmanDecoding dctDcSizeChrominanceTable;
	HuffmanDecoding dctCoeffFirstRunTable;
	HuffmanDecoding dctCoeffFirstLevelTable;
	HuffmanDecoding dctCoeffNextRunTable;
	HuffmanDecoding dctCoeffNextLevelTable;
	Block scan;
	MPEGDecoder(){
		
		this->macroblockAddressIncrementTable.add(1, 0x1, 1);
		this->macroblockAddressIncrementTable.add(2, 0x3, 3);
		this->macroblockAddressIncrementTable.add(3, 0x2, 3);
		this->macroblockAddressIncrementTable.add(4, 0x3, 4);
		this->macroblockAddressIncrementTable.add(5, 0x2, 4);
		this->macroblockAddressIncrementTable.add(6, 0x3, 5);
		this->macroblockAddressIncrementTable.add(7, 0x2, 5);
		this->macroblockAddressIncrementTable.add(8, 0x7, 7);
		this->macroblockAddressIncrementTable.add(9, 0x6, 7);
		this->macroblockAddressIncrementTable.add(10, 0xB, 8);
		this->macroblockAddressIncrementTable.add(11, 0xA, 8);
		this->macroblockAddressIncrementTable.add(12, 0x9, 8);
		this->macroblockAddressIncrementTable.add(13, 0x8, 8);
		this->macroblockAddressIncrementTable.add(14, 0x7, 8);
		this->macroblockAddressIncrementTable.add(15, 0x6, 8);
		this->macroblockAddressIncrementTable.add(16, 0x17, 10);
		this->macroblockAddressIncrementTable.add(17, 0x16, 10);
		this->macroblockAddressIncrementTable.add(18, 0x15, 10);
		this->macroblockAddressIncrementTable.add(19, 0x14, 10);
		this->macroblockAddressIncrementTable.add(20, 0x13, 10);
		this->macroblockAddressIncrementTable.add(21, 0x12, 10);
		this->macroblockAddressIncrementTable.add(22, 0x23, 11);
		this->macroblockAddressIncrementTable.add(23, 0x22, 11);
		this->macroblockAddressIncrementTable.add(24, 0x21, 11);
		this->macroblockAddressIncrementTable.add(25, 0x20, 11);
		this->macroblockAddressIncrementTable.add(26, 0x1F, 11);
		this->macroblockAddressIncrementTable.add(27, 0x1E, 11);
		this->macroblockAddressIncrementTable.add(28, 0x1D, 11);
		this->macroblockAddressIncrementTable.add(29, 0x1C, 11);
		this->macroblockAddressIncrementTable.add(30, 0x1B, 11);
		this->macroblockAddressIncrementTable.add(31, 0x1A, 11);
		this->macroblockAddressIncrementTable.add(32, 0x19, 11);
		this->macroblockAddressIncrementTable.add(33, 0x18, 11);
		this->macroblockAddressIncrementTable.initialize();
		
		this->macroblockTypeITable.add(1, 0x1, 1);
		this->macroblockTypeITable.add(2, 0x1, 2);
		this->macroblockTypeITable.initialize();
		
		this->macroblockTypePTable.add(1, 0x1, 1);
		this->macroblockTypePTable.add(2, 0x1, 2);
		this->macroblockTypePTable.add(3, 0x1, 3);
		this->macroblockTypePTable.add(4, 0x3, 5);
		this->macroblockTypePTable.add(5, 0x2, 5);
		this->macroblockTypePTable.add(6, 0x1, 5);
		this->macroblockTypePTable.add(7, 0x1, 6);
		this->macroblockTypePTable.initialize();
		
		this->macroblockTypeBTable.add(1, 0x2, 2);
		this->macroblockTypeBTable.add(2, 0x3, 2);
		this->macroblockTypeBTable.add(3, 0x2, 3);
		this->macroblockTypeBTable.add(4, 0x3, 3);
		this->macroblockTypeBTable.add(5, 0x2, 4);
		this->macroblockTypeBTable.add(6, 0x3, 4);
		this->macroblockTypeBTable.add(7, 0x3, 5);
		this->macroblockTypeBTable.add(8, 0x2, 5);
		this->macroblockTypeBTable.add(9, 0x3, 6);
		this->macroblockTypeBTable.add(10, 0x2, 6);
		this->macroblockTypeBTable.add(11, 0x1, 6);
		this->macroblockTypeBTable.initialize();
		
		this->motionVectorTable.add(-16, 0x19, 11);
		this->motionVectorTable.add(-15, 0x1B, 11);
		this->motionVectorTable.add(-14, 0x1D, 11);
		this->motionVectorTable.add(-13, 0x1F, 11);
		this->motionVectorTable.add(-12, 0x21, 11);
		this->motionVectorTable.add(-11, 0x23, 11);
		this->motionVectorTable.add(-10, 0x13, 10);
		this->motionVectorTable.add(-9, 0x15, 10);
		this->motionVectorTable.add(-8, 0x17, 10);
		this->motionVectorTable.add(-7, 0x7, 8);
		this->motionVectorTable.add(-6, 0x9, 8);
		this->motionVectorTable.add(-5, 0xB, 8);
		this->motionVectorTable.add(-4, 0x7, 7);
		this->motionVectorTable.add(-3, 0x3, 5);
		this->motionVectorTable.add(-2, 0x3, 4);
		this->motionVectorTable.add(-1, 0x3, 3);
		this->motionVectorTable.add(0, 0x1, 1);
		this->motionVectorTable.add(1, 0x2, 3);
		this->motionVectorTable.add(2, 0x2, 4);
		this->motionVectorTable.add(3, 0x2, 5);
		this->motionVectorTable.add(4, 0x6, 7);
		this->motionVectorTable.add(5, 0xA, 8);
		this->motionVectorTable.add(6, 0x8, 8);
		this->motionVectorTable.add(7, 0x6, 8);
		this->motionVectorTable.add(8, 0x16, 10);
		this->motionVectorTable.add(9, 0x14, 10);
		this->motionVectorTable.add(10, 0x12, 10);
		this->motionVectorTable.add(11, 0x22, 11);
		this->motionVectorTable.add(12, 0x20, 11);
		this->motionVectorTable.add(13, 0x1E, 11);
		this->motionVectorTable.add(14, 0x1C, 11);
		this->motionVectorTable.add(15, 0x1A, 11);
		this->motionVectorTable.add(16, 0x18, 11);
		this->motionVectorTable.initialize();
		
		this->macroblockPatternTable.add(60, 0x7, 3);
		this->macroblockPatternTable.add(4, 0xD, 4);
		this->macroblockPatternTable.add(8, 0xC, 4);
		this->macroblockPatternTable.add(16, 0xB, 4);
		this->macroblockPatternTable.add(32, 0xA, 4);
		this->macroblockPatternTable.add(12, 0x13, 5);
		this->macroblockPatternTable.add(48, 0x12, 5);
		this->macroblockPatternTable.add(20, 0x11, 5);
		this->macroblockPatternTable.add(40, 0x10, 5);
		this->macroblockPatternTable.add(28, 0xF, 5);
		this->macroblockPatternTable.add(44, 0xE, 5);
		this->macroblockPatternTable.add(52, 0xD, 5);
		this->macroblockPatternTable.add(56, 0xC, 5);
		this->macroblockPatternTable.add(1, 0xB, 5);
		this->macroblockPatternTable.add(61, 0xA, 5);
		this->macroblockPatternTable.add(2, 0x9, 5);
		this->macroblockPatternTable.add(62, 0x8, 5);
		this->macroblockPatternTable.add(24, 0xF, 6);
		this->macroblockPatternTable.add(36, 0xE, 6);
		this->macroblockPatternTable.add(3, 0xD, 6);
		this->macroblockPatternTable.add(63, 0xC, 6);
		this->macroblockPatternTable.add(5, 0x17, 7);
		this->macroblockPatternTable.add(9, 0x16, 7);
		this->macroblockPatternTable.add(17, 0x15, 7);
		this->macroblockPatternTable.add(33, 0x14, 7);
		this->macroblockPatternTable.add(6, 0x13, 7);
		this->macroblockPatternTable.add(10, 0x12, 7);
		this->macroblockPatternTable.add(18, 0x11, 7);
		this->macroblockPatternTable.add(34, 0x10, 7);
		this->macroblockPatternTable.add(7, 0x1F, 8);
		this->macroblockPatternTable.add(11, 0x1E, 8);
		this->macroblockPatternTable.add(19, 0x1D, 8);
		this->macroblockPatternTable.add(35, 0x1C, 8);
		this->macroblockPatternTable.add(13, 0x1B, 8);
		this->macroblockPatternTable.add(49, 0x1A, 8);
		this->macroblockPatternTable.add(21, 0x19, 8);
		this->macroblockPatternTable.add(41, 0x18, 8);
		this->macroblockPatternTable.add(14, 0x17, 8);
		this->macroblockPatternTable.add(50, 0x16, 8);
		this->macroblockPatternTable.add(22, 0x15, 8);
		this->macroblockPatternTable.add(42, 0x14, 8);
		this->macroblockPatternTable.add(15, 0x13, 8);
		this->macroblockPatternTable.add(51, 0x12, 8);
		this->macroblockPatternTable.add(23, 0x11, 8);
		this->macroblockPatternTable.add(43, 0x10, 8);
		this->macroblockPatternTable.add(25, 0xF, 8);
		this->macroblockPatternTable.add(37, 0xE, 8);
		this->macroblockPatternTable.add(26, 0xD, 8);
		this->macroblockPatternTable.add(38, 0xC, 8);
		this->macroblockPatternTable.add(29, 0xB, 8);
		this->macroblockPatternTable.add(45, 0xA, 8);
		this->macroblockPatternTable.add(53, 0x9, 8);
		this->macroblockPatternTable.add(57, 0x8, 8);
		this->macroblockPatternTable.add(30, 0x7, 8);
		this->macroblockPatternTable.add(46, 0x6, 8);
		this->macroblockPatternTable.add(54, 0x5, 8);
		this->macroblockPatternTable.add(58, 0x4, 8);
		this->macroblockPatternTable.add(31, 0x7, 9);
		this->macroblockPatternTable.add(47, 0x6, 9);
		this->macroblockPatternTable.add(55, 0x5, 9);
		this->macroblockPatternTable.add(59, 0x4, 9);
		this->macroblockPatternTable.add(27, 0x3, 9);
		this->macroblockPatternTable.add(39, 0x2, 9);
		this->macroblockPatternTable.initialize();
		
		this->dctDcSizeLuminanceTable.add(0, 0x4, 3);
		this->dctDcSizeLuminanceTable.add(1, 0x0, 2);
		this->dctDcSizeLuminanceTable.add(2, 0x1, 2);
		this->dctDcSizeLuminanceTable.add(3, 0x5, 3);
		this->dctDcSizeLuminanceTable.add(4, 0x6, 3);
		this->dctDcSizeLuminanceTable.add(5, 0xE, 4);
		this->dctDcSizeLuminanceTable.add(6, 0x1E, 5);
		this->dctDcSizeLuminanceTable.add(7, 0x3E, 6);
		this->dctDcSizeLuminanceTable.add(8, 0x7E, 7);
		this->dctDcSizeLuminanceTable.initialize();
		
		this->dctDcSizeChrominanceTable.add(0, 0x0, 2);
		this->dctDcSizeChrominanceTable.add(1, 0x1, 2);
		this->dctDcSizeChrominanceTable.add(2, 0x2, 2);
		this->dctDcSizeChrominanceTable.add(3, 0x6, 3);
		this->dctDcSizeChrominanceTable.add(4, 0xE, 4);
		this->dctDcSizeChrominanceTable.add(5, 0x1E, 5);
		this->dctDcSizeChrominanceTable.add(6, 0x3E, 6);
		this->dctDcSizeChrominanceTable.add(7, 0x7E, 7);
		this->dctDcSizeChrominanceTable.add(8, 0xFE, 8);
		this->dctDcSizeChrominanceTable.initialize();
		
		this->dctCoeffFirstRunTable.add(0, 0x2, 2);           
		this->dctCoeffFirstRunTable.add(0, 0x3, 2);           
		this->dctCoeffFirstRunTable.add(1, 0x6, 4);           
		this->dctCoeffFirstRunTable.add(1, 0x7, 4);           
		this->dctCoeffFirstRunTable.add(0, 0x8, 5);           
		this->dctCoeffFirstRunTable.add(0, 0x9, 5);           
		this->dctCoeffFirstRunTable.add(2, 0xA, 5);           
		this->dctCoeffFirstRunTable.add(2, 0xB, 5);           
		this->dctCoeffFirstRunTable.add(0, 0xA, 6);           
		this->dctCoeffFirstRunTable.add(0, 0xB, 6);           
		this->dctCoeffFirstRunTable.add(3, 0xE, 6);           
		this->dctCoeffFirstRunTable.add(3, 0xF, 6);           
		this->dctCoeffFirstRunTable.add(4, 0xC, 6);           
		this->dctCoeffFirstRunTable.add(4, 0xD, 6);           
		this->dctCoeffFirstRunTable.add(1, 0xC, 7);           
		this->dctCoeffFirstRunTable.add(1, 0xD, 7);           
		this->dctCoeffFirstRunTable.add(5, 0xE, 7);           
		this->dctCoeffFirstRunTable.add(5, 0xF, 7);           
		this->dctCoeffFirstRunTable.add(6, 0xA, 7);           
		this->dctCoeffFirstRunTable.add(6, 0xB, 7);           
		this->dctCoeffFirstRunTable.add(7, 0x8, 7);           
		this->dctCoeffFirstRunTable.add(7, 0x9, 7);           
		this->dctCoeffFirstRunTable.add(0, 0xC, 8);           
		this->dctCoeffFirstRunTable.add(0, 0xD, 8);           
		this->dctCoeffFirstRunTable.add(2, 0x8, 8);           
		this->dctCoeffFirstRunTable.add(2, 0x9, 8);           
		this->dctCoeffFirstRunTable.add(8, 0xE, 8);           
		this->dctCoeffFirstRunTable.add(8, 0xF, 8);           
		this->dctCoeffFirstRunTable.add(9, 0xA, 8);           
		this->dctCoeffFirstRunTable.add(9, 0xB, 8);           
		this->dctCoeffFirstRunTable.add(-1, 0x1, 6);          
		this->dctCoeffFirstRunTable.add(0, 0x4C, 9);          
		this->dctCoeffFirstRunTable.add(0, 0x4D, 9);          
		this->dctCoeffFirstRunTable.add(0, 0x42, 9);          
		this->dctCoeffFirstRunTable.add(0, 0x43, 9);          
		this->dctCoeffFirstRunTable.add(1, 0x4A, 9);          
		this->dctCoeffFirstRunTable.add(1, 0x4B, 9);          
		this->dctCoeffFirstRunTable.add(3, 0x48, 9);          
		this->dctCoeffFirstRunTable.add(3, 0x49, 9);          
		this->dctCoeffFirstRunTable.add(10, 0x4E, 9);         
		this->dctCoeffFirstRunTable.add(10, 0x4F, 9);         
		this->dctCoeffFirstRunTable.add(11, 0x46, 9);         
		this->dctCoeffFirstRunTable.add(11, 0x47, 9);         
		this->dctCoeffFirstRunTable.add(12, 0x44, 9);         
		this->dctCoeffFirstRunTable.add(12, 0x45, 9);         
		this->dctCoeffFirstRunTable.add(13, 0x40, 9);         
		this->dctCoeffFirstRunTable.add(13, 0x41, 9);         
		this->dctCoeffFirstRunTable.add(0, 0x14, 11);         
		this->dctCoeffFirstRunTable.add(0, 0x15, 11);         
		this->dctCoeffFirstRunTable.add(1, 0x18, 11);         
		this->dctCoeffFirstRunTable.add(1, 0x19, 11);         
		this->dctCoeffFirstRunTable.add(2, 0x16, 11);         
		this->dctCoeffFirstRunTable.add(2, 0x17, 11);         
		this->dctCoeffFirstRunTable.add(4, 0x1E, 11);         
		this->dctCoeffFirstRunTable.add(4, 0x1F, 11);         
		this->dctCoeffFirstRunTable.add(5, 0x12, 11);         
		this->dctCoeffFirstRunTable.add(5, 0x13, 11);         
		this->dctCoeffFirstRunTable.add(14, 0x1C, 11);        
		this->dctCoeffFirstRunTable.add(14, 0x1D, 11);        
		this->dctCoeffFirstRunTable.add(15, 0x1A, 11);        
		this->dctCoeffFirstRunTable.add(15, 0x1B, 11);        
		this->dctCoeffFirstRunTable.add(16, 0x10, 11);        
		this->dctCoeffFirstRunTable.add(16, 0x11, 11);        
		this->dctCoeffFirstRunTable.add(0, 0x3A, 13);         
		this->dctCoeffFirstRunTable.add(0, 0x3B, 13);         
		this->dctCoeffFirstRunTable.add(0, 0x30, 13);                                                
		this->dctCoeffFirstRunTable.add(0, 0x31, 13);                                                 
		this->dctCoeffFirstRunTable.add(0, 0x26, 13);         
		this->dctCoeffFirstRunTable.add(0, 0x27, 13);         
		this->dctCoeffFirstRunTable.add(0, 0x20, 13);         
		this->dctCoeffFirstRunTable.add(0, 0x21, 13);         
		this->dctCoeffFirstRunTable.add(1, 0x36, 13);         
		this->dctCoeffFirstRunTable.add(1, 0x37, 13);         
		this->dctCoeffFirstRunTable.add(2, 0x28, 13);         
		this->dctCoeffFirstRunTable.add(2, 0x29, 13);         
		this->dctCoeffFirstRunTable.add(3, 0x38, 13);         
		this->dctCoeffFirstRunTable.add(3, 0x39, 13);         
		this->dctCoeffFirstRunTable.add(4, 0x24, 13);         
		this->dctCoeffFirstRunTable.add(4, 0x25, 13);         
		this->dctCoeffFirstRunTable.add(6, 0x3C, 13);         
		this->dctCoeffFirstRunTable.add(6, 0x3D, 13);         
		this->dctCoeffFirstRunTable.add(7, 0x2A, 13);         
		this->dctCoeffFirstRunTable.add(7, 0x2B, 13);         
		this->dctCoeffFirstRunTable.add(8, 0x22, 13);         
		this->dctCoeffFirstRunTable.add(8, 0x23, 13);         
		this->dctCoeffFirstRunTable.add(17, 0x3E, 13);        
		this->dctCoeffFirstRunTable.add(17, 0x3F, 13);        
		this->dctCoeffFirstRunTable.add(18, 0x34, 13);        
		this->dctCoeffFirstRunTable.add(18, 0x35, 13);        
		this->dctCoeffFirstRunTable.add(19, 0x32, 13);        
		this->dctCoeffFirstRunTable.add(19, 0x33, 13);        
		this->dctCoeffFirstRunTable.add(20, 0x2E, 13);        
		this->dctCoeffFirstRunTable.add(20, 0x2F, 13);        
		this->dctCoeffFirstRunTable.add(21, 0x2C, 13);        
		this->dctCoeffFirstRunTable.add(21, 0x2D, 13);        
		this->dctCoeffFirstRunTable.add(0, 0x34, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x35, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x32, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x33, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x30, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x31, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x2E, 14);         
		this->dctCoeffFirstRunTable.add(0, 0x2F, 14);         
		this->dctCoeffFirstRunTable.add(1, 0x2C, 14);         
		this->dctCoeffFirstRunTable.add(1, 0x2D, 14);         
		this->dctCoeffFirstRunTable.add(1, 0x2A, 14);         
		this->dctCoeffFirstRunTable.add(1, 0x2B, 14);         
		this->dctCoeffFirstRunTable.add(2, 0x28, 14);         
		this->dctCoeffFirstRunTable.add(2, 0x29, 14);         
		this->dctCoeffFirstRunTable.add(3, 0x26, 14);         
		this->dctCoeffFirstRunTable.add(3, 0x27, 14);         
		this->dctCoeffFirstRunTable.add(5, 0x24, 14);         
		this->dctCoeffFirstRunTable.add(5, 0x25, 14);         
		this->dctCoeffFirstRunTable.add(9, 0x22, 14);         
		this->dctCoeffFirstRunTable.add(9, 0x23, 14);         
		this->dctCoeffFirstRunTable.add(10, 0x20, 14);        
		this->dctCoeffFirstRunTable.add(10, 0x21, 14);        
		this->dctCoeffFirstRunTable.add(22, 0x3E, 14);        
		this->dctCoeffFirstRunTable.add(22, 0x3F, 14);        
		this->dctCoeffFirstRunTable.add(23, 0x3C, 14);        
		this->dctCoeffFirstRunTable.add(23, 0x3D, 14);        
		this->dctCoeffFirstRunTable.add(24, 0x3A, 14);        
		this->dctCoeffFirstRunTable.add(24, 0x3B, 14);        
		this->dctCoeffFirstRunTable.add(25, 0x38, 14);        
		this->dctCoeffFirstRunTable.add(25, 0x39, 14);        
		this->dctCoeffFirstRunTable.add(26, 0x36, 14);        
		this->dctCoeffFirstRunTable.add(26, 0x37, 14);        
		this->dctCoeffFirstRunTable.add(0, 0x3E, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x3F, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x3C, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x3D, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x3A, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x3B, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x38, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x39, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x36, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x37, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x34, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x35, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x32, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x33, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x30, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x31, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x2E, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x2F, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x2C, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x2D, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x2A, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x2B, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x28, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x29, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x26, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x27, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x24, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x25, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x22, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x23, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x20, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x21, 15);         
		this->dctCoeffFirstRunTable.add(0, 0x30, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x31, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x2E, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x2F, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x2C, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x2D, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x2A, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x2B, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x28, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x29, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x26, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x27, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x24, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x25, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x22, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x23, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x20, 16);         
		this->dctCoeffFirstRunTable.add(0, 0x21, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x3E, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x3F, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x3C, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x3D, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x3A, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x3B, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x38, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x39, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x36, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x37, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x34, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x35, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x32, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x33, 16);         
		this->dctCoeffFirstRunTable.add(1, 0x26, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x27, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x24, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x25, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x22, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x23, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x20, 17);         
		this->dctCoeffFirstRunTable.add(1, 0x21, 17);         
		this->dctCoeffFirstRunTable.add(6, 0x28, 17);         
		this->dctCoeffFirstRunTable.add(6, 0x29, 17);         
		this->dctCoeffFirstRunTable.add(11, 0x34, 17);        
		this->dctCoeffFirstRunTable.add(11, 0x35, 17);        
		this->dctCoeffFirstRunTable.add(12, 0x32, 17);        
		this->dctCoeffFirstRunTable.add(12, 0x33, 17);        
		this->dctCoeffFirstRunTable.add(13, 0x30, 17);        
		this->dctCoeffFirstRunTable.add(13, 0x31, 17);        
		this->dctCoeffFirstRunTable.add(14, 0x2E, 17);        
		this->dctCoeffFirstRunTable.add(14, 0x2F, 17);        
		this->dctCoeffFirstRunTable.add(15, 0x2C, 17);        
		this->dctCoeffFirstRunTable.add(15, 0x2D, 17);        
		this->dctCoeffFirstRunTable.add(16, 0x2A, 17);        
		this->dctCoeffFirstRunTable.add(16, 0x2B, 17);        
		this->dctCoeffFirstRunTable.add(27, 0x3E, 17);        
		this->dctCoeffFirstRunTable.add(27, 0x3F, 17);        
		this->dctCoeffFirstRunTable.add(28, 0x3C, 17);        
		this->dctCoeffFirstRunTable.add(28, 0x3D, 17);        
		this->dctCoeffFirstRunTable.add(29, 0x3A, 17);        
		this->dctCoeffFirstRunTable.add(29, 0x3B, 17);        
		this->dctCoeffFirstRunTable.add(30, 0x38, 17);        
		this->dctCoeffFirstRunTable.add(30, 0x39, 17);        
		this->dctCoeffFirstRunTable.add(31, 0x36, 17);        
		this->dctCoeffFirstRunTable.add(31, 0x37, 17);        
		this->dctCoeffFirstRunTable.initialize();             
		
		this->dctCoeffFirstLevelTable.add(1, 0x2, 2);
		this->dctCoeffFirstLevelTable.add(-1, 0x3, 2);
		this->dctCoeffFirstLevelTable.add(1, 0x6, 4);
		this->dctCoeffFirstLevelTable.add(-1, 0x7, 4);
		this->dctCoeffFirstLevelTable.add(2, 0x8, 5);
		this->dctCoeffFirstLevelTable.add(-2, 0x9, 5);
		this->dctCoeffFirstLevelTable.add(1, 0xA, 5);
		this->dctCoeffFirstLevelTable.add(-1, 0xB, 5);
		this->dctCoeffFirstLevelTable.add(3, 0xA, 6);
		this->dctCoeffFirstLevelTable.add(-3, 0xB, 6);
		this->dctCoeffFirstLevelTable.add(1, 0xE, 6);
		this->dctCoeffFirstLevelTable.add(-1, 0xF, 6);
		this->dctCoeffFirstLevelTable.add(1, 0xC, 6);
		this->dctCoeffFirstLevelTable.add(-1, 0xD, 6);
		this->dctCoeffFirstLevelTable.add(2, 0xC, 7);
		this->dctCoeffFirstLevelTable.add(-2, 0xD, 7);
		this->dctCoeffFirstLevelTable.add(1, 0xE, 7);
		this->dctCoeffFirstLevelTable.add(-1, 0xF, 7);
		this->dctCoeffFirstLevelTable.add(1, 0xA, 7);
		this->dctCoeffFirstLevelTable.add(-1, 0xB, 7);
		this->dctCoeffFirstLevelTable.add(1, 0x8, 7);
		this->dctCoeffFirstLevelTable.add(-1, 0x9, 7);
		this->dctCoeffFirstLevelTable.add(4, 0xC, 8);
		this->dctCoeffFirstLevelTable.add(-4, 0xD, 8);
		this->dctCoeffFirstLevelTable.add(2, 0x8, 8);
		this->dctCoeffFirstLevelTable.add(-2, 0x9, 8);
		this->dctCoeffFirstLevelTable.add(1, 0xE, 8);
		this->dctCoeffFirstLevelTable.add(-1, 0xF, 8);
		this->dctCoeffFirstLevelTable.add(1, 0xA, 8);
		this->dctCoeffFirstLevelTable.add(-1, 0xB, 8);
		this->dctCoeffFirstLevelTable.add(0, 0x1, 6);
		this->dctCoeffFirstLevelTable.add(5, 0x4C, 9);
		this->dctCoeffFirstLevelTable.add(-5, 0x4D, 9);
		this->dctCoeffFirstLevelTable.add(6, 0x42, 9);
		this->dctCoeffFirstLevelTable.add(-6, 0x43, 9);
		this->dctCoeffFirstLevelTable.add(3, 0x4A, 9);
		this->dctCoeffFirstLevelTable.add(-3, 0x4B, 9);
		this->dctCoeffFirstLevelTable.add(2, 0x48, 9);
		this->dctCoeffFirstLevelTable.add(-2, 0x49, 9);
		this->dctCoeffFirstLevelTable.add(1, 0x4E, 9);
		this->dctCoeffFirstLevelTable.add(-1, 0x4F, 9);
		this->dctCoeffFirstLevelTable.add(1, 0x46, 9);
		this->dctCoeffFirstLevelTable.add(-1, 0x47, 9);
		this->dctCoeffFirstLevelTable.add(1, 0x44, 9);
		this->dctCoeffFirstLevelTable.add(-1, 0x45, 9);
		this->dctCoeffFirstLevelTable.add(1, 0x40, 9);
		this->dctCoeffFirstLevelTable.add(-1, 0x41, 9);
		this->dctCoeffFirstLevelTable.add(7, 0x14, 11);
		this->dctCoeffFirstLevelTable.add(-7, 0x15, 11);
		this->dctCoeffFirstLevelTable.add(4, 0x18, 11);
		this->dctCoeffFirstLevelTable.add(-4, 0x19, 11);
		this->dctCoeffFirstLevelTable.add(3, 0x16, 11);
		this->dctCoeffFirstLevelTable.add(-3, 0x17, 11);
		this->dctCoeffFirstLevelTable.add(2, 0x1E, 11);
		this->dctCoeffFirstLevelTable.add(-2, 0x1F, 11);
		this->dctCoeffFirstLevelTable.add(2, 0x12, 11);
		this->dctCoeffFirstLevelTable.add(-2, 0x13, 11);
		this->dctCoeffFirstLevelTable.add(1, 0x1C, 11);
		this->dctCoeffFirstLevelTable.add(-1, 0x1D, 11);
		this->dctCoeffFirstLevelTable.add(1, 0x1A, 11);
		this->dctCoeffFirstLevelTable.add(-1, 0x1B, 11);
		this->dctCoeffFirstLevelTable.add(1, 0x10, 11);
		this->dctCoeffFirstLevelTable.add(-1, 0x11, 11);
		this->dctCoeffFirstLevelTable.add(8, 0x3A, 13);
		this->dctCoeffFirstLevelTable.add(-8, 0x3B, 13);
		this->dctCoeffFirstLevelTable.add(9, 0x30, 13);      
		this->dctCoeffFirstLevelTable.add(-9, 0x31, 13);     
		this->dctCoeffFirstLevelTable.add(10, 0x26, 13);
		this->dctCoeffFirstLevelTable.add(-10, 0x27, 13);
		this->dctCoeffFirstLevelTable.add(11, 0x20, 13);
		this->dctCoeffFirstLevelTable.add(-11, 0x21, 13);
		this->dctCoeffFirstLevelTable.add(5, 0x36, 13);
		this->dctCoeffFirstLevelTable.add(-5, 0x37, 13);
		this->dctCoeffFirstLevelTable.add(4, 0x28, 13);
		this->dctCoeffFirstLevelTable.add(-4, 0x29, 13);
		this->dctCoeffFirstLevelTable.add(3, 0x38, 13);
		this->dctCoeffFirstLevelTable.add(-3, 0x39, 13);
		this->dctCoeffFirstLevelTable.add(3, 0x24, 13);
		this->dctCoeffFirstLevelTable.add(-3, 0x25, 13);
		this->dctCoeffFirstLevelTable.add(2, 0x3C, 13);
		this->dctCoeffFirstLevelTable.add(-2, 0x3D, 13);
		this->dctCoeffFirstLevelTable.add(2, 0x2A, 13);
		this->dctCoeffFirstLevelTable.add(-2, 0x2B, 13);
		this->dctCoeffFirstLevelTable.add(2, 0x22, 13);
		this->dctCoeffFirstLevelTable.add(-2, 0x23, 13);
		this->dctCoeffFirstLevelTable.add(1, 0x3E, 13);
		this->dctCoeffFirstLevelTable.add(-1, 0x3F, 13);
		this->dctCoeffFirstLevelTable.add(1, 0x34, 13);
		this->dctCoeffFirstLevelTable.add(-1, 0x35, 13);
		this->dctCoeffFirstLevelTable.add(1, 0x32, 13);
		this->dctCoeffFirstLevelTable.add(-1, 0x33, 13);
		this->dctCoeffFirstLevelTable.add(1, 0x2E, 13);
		this->dctCoeffFirstLevelTable.add(-1, 0x2F, 13);
		this->dctCoeffFirstLevelTable.add(1, 0x2C, 13);
		this->dctCoeffFirstLevelTable.add(-1, 0x2D, 13);
		this->dctCoeffFirstLevelTable.add(12, 0x34, 14);
		this->dctCoeffFirstLevelTable.add(-12, 0x35, 14);
		this->dctCoeffFirstLevelTable.add(13, 0x32, 14);
		this->dctCoeffFirstLevelTable.add(-13, 0x33, 14);
		this->dctCoeffFirstLevelTable.add(14, 0x30, 14);
		this->dctCoeffFirstLevelTable.add(-14, 0x31, 14);
		this->dctCoeffFirstLevelTable.add(15, 0x2E, 14);
		this->dctCoeffFirstLevelTable.add(-15, 0x2F, 14);
		this->dctCoeffFirstLevelTable.add(6, 0x2C, 14);
		this->dctCoeffFirstLevelTable.add(-6, 0x2D, 14);
		this->dctCoeffFirstLevelTable.add(7, 0x2A, 14);
		this->dctCoeffFirstLevelTable.add(-7, 0x2B, 14);
		this->dctCoeffFirstLevelTable.add(5, 0x28, 14);
		this->dctCoeffFirstLevelTable.add(-5, 0x29, 14);
		this->dctCoeffFirstLevelTable.add(4, 0x26, 14);
		this->dctCoeffFirstLevelTable.add(-4, 0x27, 14);
		this->dctCoeffFirstLevelTable.add(3, 0x24, 14);
		this->dctCoeffFirstLevelTable.add(-3, 0x25, 14);
		this->dctCoeffFirstLevelTable.add(2, 0x22, 14);
		this->dctCoeffFirstLevelTable.add(-2, 0x23, 14);
		this->dctCoeffFirstLevelTable.add(2, 0x20, 14);
		this->dctCoeffFirstLevelTable.add(-2, 0x21, 14);
		this->dctCoeffFirstLevelTable.add(1, 0x3E, 14);
		this->dctCoeffFirstLevelTable.add(-1, 0x3F, 14);
		this->dctCoeffFirstLevelTable.add(1, 0x3C, 14);
		this->dctCoeffFirstLevelTable.add(-1, 0x3D, 14);
		this->dctCoeffFirstLevelTable.add(1, 0x3A, 14);
		this->dctCoeffFirstLevelTable.add(-1, 0x3B, 14);
		this->dctCoeffFirstLevelTable.add(1, 0x38, 14);
		this->dctCoeffFirstLevelTable.add(-1, 0x39, 14);
		this->dctCoeffFirstLevelTable.add(1, 0x36, 14);
		this->dctCoeffFirstLevelTable.add(-1, 0x37, 14);
		this->dctCoeffFirstLevelTable.add(16, 0x3E, 15);
		this->dctCoeffFirstLevelTable.add(-16, 0x3F, 15);
		this->dctCoeffFirstLevelTable.add(17, 0x3C, 15);
		this->dctCoeffFirstLevelTable.add(-17, 0x3D, 15);
		this->dctCoeffFirstLevelTable.add(18, 0x3A, 15);
		this->dctCoeffFirstLevelTable.add(-18, 0x3B, 15);
		this->dctCoeffFirstLevelTable.add(19, 0x38, 15);
		this->dctCoeffFirstLevelTable.add(-19, 0x39, 15);
		this->dctCoeffFirstLevelTable.add(20, 0x36, 15);
		this->dctCoeffFirstLevelTable.add(-20, 0x37, 15);
		this->dctCoeffFirstLevelTable.add(21, 0x34, 15);
		this->dctCoeffFirstLevelTable.add(-21, 0x35, 15);
		this->dctCoeffFirstLevelTable.add(22, 0x32, 15);
		this->dctCoeffFirstLevelTable.add(-22, 0x33, 15);
		this->dctCoeffFirstLevelTable.add(23, 0x30, 15);
		this->dctCoeffFirstLevelTable.add(-23, 0x31, 15);
		this->dctCoeffFirstLevelTable.add(24, 0x2E, 15);
		this->dctCoeffFirstLevelTable.add(-24, 0x2F, 15);
		this->dctCoeffFirstLevelTable.add(25, 0x2C, 15);
		this->dctCoeffFirstLevelTable.add(-25, 0x2D, 15);
		this->dctCoeffFirstLevelTable.add(26, 0x2A, 15);
		this->dctCoeffFirstLevelTable.add(-26, 0x2B, 15);
		this->dctCoeffFirstLevelTable.add(27, 0x28, 15);
		this->dctCoeffFirstLevelTable.add(-27, 0x29, 15);
		this->dctCoeffFirstLevelTable.add(28, 0x26, 15);
		this->dctCoeffFirstLevelTable.add(-28, 0x27, 15);
		this->dctCoeffFirstLevelTable.add(29, 0x24, 15);
		this->dctCoeffFirstLevelTable.add(-29, 0x25, 15);
		this->dctCoeffFirstLevelTable.add(30, 0x22, 15);
		this->dctCoeffFirstLevelTable.add(-30, 0x23, 15);
		this->dctCoeffFirstLevelTable.add(31, 0x20, 15);
		this->dctCoeffFirstLevelTable.add(-31, 0x21, 15);
		this->dctCoeffFirstLevelTable.add(32, 0x30, 16);
		this->dctCoeffFirstLevelTable.add(-32, 0x31, 16);
		this->dctCoeffFirstLevelTable.add(33, 0x2E, 16);
		this->dctCoeffFirstLevelTable.add(-33, 0x2F, 16);
		this->dctCoeffFirstLevelTable.add(34, 0x2C, 16);
		this->dctCoeffFirstLevelTable.add(-34, 0x2D, 16);
		this->dctCoeffFirstLevelTable.add(35, 0x2A, 16);
		this->dctCoeffFirstLevelTable.add(-35, 0x2B, 16);
		this->dctCoeffFirstLevelTable.add(36, 0x28, 16);
		this->dctCoeffFirstLevelTable.add(-36, 0x29, 16);
		this->dctCoeffFirstLevelTable.add(37, 0x26, 16);
		this->dctCoeffFirstLevelTable.add(-37, 0x27, 16);
		this->dctCoeffFirstLevelTable.add(38, 0x24, 16);
		this->dctCoeffFirstLevelTable.add(-38, 0x25, 16);
		this->dctCoeffFirstLevelTable.add(39, 0x22, 16);
		this->dctCoeffFirstLevelTable.add(-39, 0x23, 16);
		this->dctCoeffFirstLevelTable.add(40, 0x20, 16);
		this->dctCoeffFirstLevelTable.add(-40, 0x21, 16);
		this->dctCoeffFirstLevelTable.add(8, 0x3E, 16);
		this->dctCoeffFirstLevelTable.add(-8, 0x3F, 16);
		this->dctCoeffFirstLevelTable.add(9, 0x3C, 16);
		this->dctCoeffFirstLevelTable.add(-9, 0x3D, 16);
		this->dctCoeffFirstLevelTable.add(10, 0x3A, 16);
		this->dctCoeffFirstLevelTable.add(-10, 0x3B, 16);
		this->dctCoeffFirstLevelTable.add(11, 0x38, 16);
		this->dctCoeffFirstLevelTable.add(-11, 0x39, 16);
		this->dctCoeffFirstLevelTable.add(12, 0x36, 16);
		this->dctCoeffFirstLevelTable.add(-12, 0x37, 16);
		this->dctCoeffFirstLevelTable.add(13, 0x34, 16);
		this->dctCoeffFirstLevelTable.add(-13, 0x35, 16);
		this->dctCoeffFirstLevelTable.add(14, 0x32, 16);
		this->dctCoeffFirstLevelTable.add(-14, 0x33, 16);
		this->dctCoeffFirstLevelTable.add(15, 0x26, 17);
		this->dctCoeffFirstLevelTable.add(-15, 0x27, 17);
		this->dctCoeffFirstLevelTable.add(16, 0x24, 17);
		this->dctCoeffFirstLevelTable.add(-16, 0x25, 17);
		this->dctCoeffFirstLevelTable.add(17, 0x22, 17);
		this->dctCoeffFirstLevelTable.add(-17, 0x23, 17);
		this->dctCoeffFirstLevelTable.add(18, 0x20, 17);
		this->dctCoeffFirstLevelTable.add(-18, 0x21, 17);
		this->dctCoeffFirstLevelTable.add(3, 0x28, 17);
		this->dctCoeffFirstLevelTable.add(-3, 0x29, 17);
		this->dctCoeffFirstLevelTable.add(2, 0x34, 17);
		this->dctCoeffFirstLevelTable.add(-2, 0x35, 17);
		this->dctCoeffFirstLevelTable.add(2, 0x32, 17);
		this->dctCoeffFirstLevelTable.add(-2, 0x33, 17);
		this->dctCoeffFirstLevelTable.add(2, 0x30, 17);
		this->dctCoeffFirstLevelTable.add(-2, 0x31, 17);
		this->dctCoeffFirstLevelTable.add(2, 0x2E, 17);
		this->dctCoeffFirstLevelTable.add(-2, 0x2F, 17);
		this->dctCoeffFirstLevelTable.add(2, 0x2C, 17);
		this->dctCoeffFirstLevelTable.add(-2, 0x2D, 17);
		this->dctCoeffFirstLevelTable.add(2, 0x2A, 17);
		this->dctCoeffFirstLevelTable.add(-2, 0x2B, 17);
		this->dctCoeffFirstLevelTable.add(1, 0x3E, 17);
		this->dctCoeffFirstLevelTable.add(-1, 0x3F, 17);
		this->dctCoeffFirstLevelTable.add(1, 0x3C, 17);
		this->dctCoeffFirstLevelTable.add(-1, 0x3D, 17);
		this->dctCoeffFirstLevelTable.add(1, 0x3A, 17);
		this->dctCoeffFirstLevelTable.add(-1, 0x3B, 17);
		this->dctCoeffFirstLevelTable.add(1, 0x38, 17);
		this->dctCoeffFirstLevelTable.add(-1, 0x39, 17);
        this->dctCoeffFirstLevelTable.add(1, 0x36, 17);
        this->dctCoeffFirstLevelTable.add(-1, 0x37, 17);
        this->dctCoeffFirstLevelTable.initialize();

		this->dctCoeffNextRunTable.add(0, 0x6, 3);           
		this->dctCoeffNextRunTable.add(0, 0x7, 3);           
		this->dctCoeffNextRunTable.add(1, 0x6, 4);           
		this->dctCoeffNextRunTable.add(1, 0x7, 4);           
		this->dctCoeffNextRunTable.add(0, 0x8, 5);           
		this->dctCoeffNextRunTable.add(0, 0x9, 5);           
		this->dctCoeffNextRunTable.add(2, 0xA, 5);           
		this->dctCoeffNextRunTable.add(2, 0xB, 5);           
		this->dctCoeffNextRunTable.add(0, 0xA, 6);           
		this->dctCoeffNextRunTable.add(0, 0xB, 6);           
		this->dctCoeffNextRunTable.add(3, 0xE, 6);           
		this->dctCoeffNextRunTable.add(3, 0xF, 6);           
		this->dctCoeffNextRunTable.add(4, 0xC, 6);           
		this->dctCoeffNextRunTable.add(4, 0xD, 6);           
		this->dctCoeffNextRunTable.add(1, 0xC, 7);           
		this->dctCoeffNextRunTable.add(1, 0xD, 7);           
		this->dctCoeffNextRunTable.add(5, 0xE, 7);           
		this->dctCoeffNextRunTable.add(5, 0xF, 7);           
		this->dctCoeffNextRunTable.add(6, 0xA, 7);           
		this->dctCoeffNextRunTable.add(6, 0xB, 7);           
		this->dctCoeffNextRunTable.add(7, 0x8, 7);           
		this->dctCoeffNextRunTable.add(7, 0x9, 7);           
		this->dctCoeffNextRunTable.add(0, 0xC, 8);           
		this->dctCoeffNextRunTable.add(0, 0xD, 8);           
		this->dctCoeffNextRunTable.add(2, 0x8, 8);           
		this->dctCoeffNextRunTable.add(2, 0x9, 8);           
		this->dctCoeffNextRunTable.add(8, 0xE, 8);           
		this->dctCoeffNextRunTable.add(8, 0xF, 8);           
		this->dctCoeffNextRunTable.add(9, 0xA, 8);           
		this->dctCoeffNextRunTable.add(9, 0xB, 8);           
		this->dctCoeffNextRunTable.add(-1, 0x1, 6);          
		this->dctCoeffNextRunTable.add(0, 0x4C, 9);          
		this->dctCoeffNextRunTable.add(0, 0x4D, 9);          
		this->dctCoeffNextRunTable.add(0, 0x42, 9);          
		this->dctCoeffNextRunTable.add(0, 0x43, 9);          
		this->dctCoeffNextRunTable.add(1, 0x4A, 9);          
		this->dctCoeffNextRunTable.add(1, 0x4B, 9);          
		this->dctCoeffNextRunTable.add(3, 0x48, 9);          
		this->dctCoeffNextRunTable.add(3, 0x49, 9);          
		this->dctCoeffNextRunTable.add(10, 0x4E, 9);         
		this->dctCoeffNextRunTable.add(10, 0x4F, 9);         
		this->dctCoeffNextRunTable.add(11, 0x46, 9);         
		this->dctCoeffNextRunTable.add(11, 0x47, 9);         
		this->dctCoeffNextRunTable.add(12, 0x44, 9);         
		this->dctCoeffNextRunTable.add(12, 0x45, 9);         
		this->dctCoeffNextRunTable.add(13, 0x40, 9);         
		this->dctCoeffNextRunTable.add(13, 0x41, 9);         
		this->dctCoeffNextRunTable.add(0, 0x14, 11);         
		this->dctCoeffNextRunTable.add(0, 0x15, 11);         
		this->dctCoeffNextRunTable.add(1, 0x18, 11);         
		this->dctCoeffNextRunTable.add(1, 0x19, 11);         
		this->dctCoeffNextRunTable.add(2, 0x16, 11);         
		this->dctCoeffNextRunTable.add(2, 0x17, 11);         
		this->dctCoeffNextRunTable.add(4, 0x1E, 11);         
		this->dctCoeffNextRunTable.add(4, 0x1F, 11);         
		this->dctCoeffNextRunTable.add(5, 0x12, 11);         
		this->dctCoeffNextRunTable.add(5, 0x13, 11);         
		this->dctCoeffNextRunTable.add(14, 0x1C, 11);        
		this->dctCoeffNextRunTable.add(14, 0x1D, 11);        
		this->dctCoeffNextRunTable.add(15, 0x1A, 11);        
		this->dctCoeffNextRunTable.add(15, 0x1B, 11);        
		this->dctCoeffNextRunTable.add(16, 0x10, 11);        
		this->dctCoeffNextRunTable.add(16, 0x11, 11);        
		this->dctCoeffNextRunTable.add(0, 0x3A, 13);         
		this->dctCoeffNextRunTable.add(0, 0x3B, 13);         
		this->dctCoeffNextRunTable.add(0, 0x30, 13);                                                
		this->dctCoeffNextRunTable.add(0, 0x31, 13);                                                 
		this->dctCoeffNextRunTable.add(0, 0x26, 13);         
		this->dctCoeffNextRunTable.add(0, 0x27, 13);         
		this->dctCoeffNextRunTable.add(0, 0x20, 13);         
		this->dctCoeffNextRunTable.add(0, 0x21, 13);         
		this->dctCoeffNextRunTable.add(1, 0x36, 13);         
		this->dctCoeffNextRunTable.add(1, 0x37, 13);         
		this->dctCoeffNextRunTable.add(2, 0x28, 13);         
		this->dctCoeffNextRunTable.add(2, 0x29, 13);         
		this->dctCoeffNextRunTable.add(3, 0x38, 13);         
		this->dctCoeffNextRunTable.add(3, 0x39, 13);         
		this->dctCoeffNextRunTable.add(4, 0x24, 13);         
		this->dctCoeffNextRunTable.add(4, 0x25, 13);         
		this->dctCoeffNextRunTable.add(6, 0x3C, 13);         
		this->dctCoeffNextRunTable.add(6, 0x3D, 13);         
		this->dctCoeffNextRunTable.add(7, 0x2A, 13);         
		this->dctCoeffNextRunTable.add(7, 0x2B, 13);         
		this->dctCoeffNextRunTable.add(8, 0x22, 13);         
		this->dctCoeffNextRunTable.add(8, 0x23, 13);         
		this->dctCoeffNextRunTable.add(17, 0x3E, 13);        
		this->dctCoeffNextRunTable.add(17, 0x3F, 13);        
		this->dctCoeffNextRunTable.add(18, 0x34, 13);        
		this->dctCoeffNextRunTable.add(18, 0x35, 13);        
		this->dctCoeffNextRunTable.add(19, 0x32, 13);        
		this->dctCoeffNextRunTable.add(19, 0x33, 13);        
		this->dctCoeffNextRunTable.add(20, 0x2E, 13);        
		this->dctCoeffNextRunTable.add(20, 0x2F, 13);        
		this->dctCoeffNextRunTable.add(21, 0x2C, 13);        
		this->dctCoeffNextRunTable.add(21, 0x2D, 13);        
		this->dctCoeffNextRunTable.add(0, 0x34, 14);         
		this->dctCoeffNextRunTable.add(0, 0x35, 14);         
		this->dctCoeffNextRunTable.add(0, 0x32, 14);         
		this->dctCoeffNextRunTable.add(0, 0x33, 14);         
		this->dctCoeffNextRunTable.add(0, 0x30, 14);         
		this->dctCoeffNextRunTable.add(0, 0x31, 14);         
		this->dctCoeffNextRunTable.add(0, 0x2E, 14);         
		this->dctCoeffNextRunTable.add(0, 0x2F, 14);         
		this->dctCoeffNextRunTable.add(1, 0x2C, 14);         
		this->dctCoeffNextRunTable.add(1, 0x2D, 14);         
		this->dctCoeffNextRunTable.add(1, 0x2A, 14);         
		this->dctCoeffNextRunTable.add(1, 0x2B, 14);         
		this->dctCoeffNextRunTable.add(2, 0x28, 14);         
		this->dctCoeffNextRunTable.add(2, 0x29, 14);         
		this->dctCoeffNextRunTable.add(3, 0x26, 14);         
		this->dctCoeffNextRunTable.add(3, 0x27, 14);         
		this->dctCoeffNextRunTable.add(5, 0x24, 14);         
		this->dctCoeffNextRunTable.add(5, 0x25, 14);         
		this->dctCoeffNextRunTable.add(9, 0x22, 14);         
		this->dctCoeffNextRunTable.add(9, 0x23, 14);         
		this->dctCoeffNextRunTable.add(10, 0x20, 14);        
		this->dctCoeffNextRunTable.add(10, 0x21, 14);        
		this->dctCoeffNextRunTable.add(22, 0x3E, 14);        
		this->dctCoeffNextRunTable.add(22, 0x3F, 14);        
		this->dctCoeffNextRunTable.add(23, 0x3C, 14);        
		this->dctCoeffNextRunTable.add(23, 0x3D, 14);        
		this->dctCoeffNextRunTable.add(24, 0x3A, 14);        
		this->dctCoeffNextRunTable.add(24, 0x3B, 14);        
		this->dctCoeffNextRunTable.add(25, 0x38, 14);        
		this->dctCoeffNextRunTable.add(25, 0x39, 14);        
		this->dctCoeffNextRunTable.add(26, 0x36, 14);        
		this->dctCoeffNextRunTable.add(26, 0x37, 14);        
		this->dctCoeffNextRunTable.add(0, 0x3E, 15);         
		this->dctCoeffNextRunTable.add(0, 0x3F, 15);         
		this->dctCoeffNextRunTable.add(0, 0x3C, 15);         
		this->dctCoeffNextRunTable.add(0, 0x3D, 15);         
		this->dctCoeffNextRunTable.add(0, 0x3A, 15);         
		this->dctCoeffNextRunTable.add(0, 0x3B, 15);         
		this->dctCoeffNextRunTable.add(0, 0x38, 15);         
		this->dctCoeffNextRunTable.add(0, 0x39, 15);         
		this->dctCoeffNextRunTable.add(0, 0x36, 15);         
		this->dctCoeffNextRunTable.add(0, 0x37, 15);         
		this->dctCoeffNextRunTable.add(0, 0x34, 15);         
		this->dctCoeffNextRunTable.add(0, 0x35, 15);         
		this->dctCoeffNextRunTable.add(0, 0x32, 15);         
		this->dctCoeffNextRunTable.add(0, 0x33, 15);         
		this->dctCoeffNextRunTable.add(0, 0x30, 15);         
		this->dctCoeffNextRunTable.add(0, 0x31, 15);         
		this->dctCoeffNextRunTable.add(0, 0x2E, 15);         
		this->dctCoeffNextRunTable.add(0, 0x2F, 15);         
		this->dctCoeffNextRunTable.add(0, 0x2C, 15);         
		this->dctCoeffNextRunTable.add(0, 0x2D, 15);         
		this->dctCoeffNextRunTable.add(0, 0x2A, 15);         
		this->dctCoeffNextRunTable.add(0, 0x2B, 15);         
		this->dctCoeffNextRunTable.add(0, 0x28, 15);         
		this->dctCoeffNextRunTable.add(0, 0x29, 15);         
		this->dctCoeffNextRunTable.add(0, 0x26, 15);         
		this->dctCoeffNextRunTable.add(0, 0x27, 15);         
		this->dctCoeffNextRunTable.add(0, 0x24, 15);         
		this->dctCoeffNextRunTable.add(0, 0x25, 15);         
		this->dctCoeffNextRunTable.add(0, 0x22, 15);         
		this->dctCoeffNextRunTable.add(0, 0x23, 15);         
		this->dctCoeffNextRunTable.add(0, 0x20, 15);         
		this->dctCoeffNextRunTable.add(0, 0x21, 15);         
		this->dctCoeffNextRunTable.add(0, 0x30, 16);         
		this->dctCoeffNextRunTable.add(0, 0x31, 16);         
		this->dctCoeffNextRunTable.add(0, 0x2E, 16);         
		this->dctCoeffNextRunTable.add(0, 0x2F, 16);         
		this->dctCoeffNextRunTable.add(0, 0x2C, 16);         
		this->dctCoeffNextRunTable.add(0, 0x2D, 16);         
		this->dctCoeffNextRunTable.add(0, 0x2A, 16);         
		this->dctCoeffNextRunTable.add(0, 0x2B, 16);         
		this->dctCoeffNextRunTable.add(0, 0x28, 16);         
		this->dctCoeffNextRunTable.add(0, 0x29, 16);         
		this->dctCoeffNextRunTable.add(0, 0x26, 16);         
		this->dctCoeffNextRunTable.add(0, 0x27, 16);         
		this->dctCoeffNextRunTable.add(0, 0x24, 16);         
		this->dctCoeffNextRunTable.add(0, 0x25, 16);         
		this->dctCoeffNextRunTable.add(0, 0x22, 16);         
		this->dctCoeffNextRunTable.add(0, 0x23, 16);         
		this->dctCoeffNextRunTable.add(0, 0x20, 16);         
		this->dctCoeffNextRunTable.add(0, 0x21, 16);         
		this->dctCoeffNextRunTable.add(1, 0x3E, 16);         
		this->dctCoeffNextRunTable.add(1, 0x3F, 16);         
		this->dctCoeffNextRunTable.add(1, 0x3C, 16);         
		this->dctCoeffNextRunTable.add(1, 0x3D, 16);         
		this->dctCoeffNextRunTable.add(1, 0x3A, 16);         
		this->dctCoeffNextRunTable.add(1, 0x3B, 16);         
		this->dctCoeffNextRunTable.add(1, 0x38, 16);         
		this->dctCoeffNextRunTable.add(1, 0x39, 16);         
		this->dctCoeffNextRunTable.add(1, 0x36, 16);         
		this->dctCoeffNextRunTable.add(1, 0x37, 16);         
		this->dctCoeffNextRunTable.add(1, 0x34, 16);         
		this->dctCoeffNextRunTable.add(1, 0x35, 16);         
		this->dctCoeffNextRunTable.add(1, 0x32, 16);         
		this->dctCoeffNextRunTable.add(1, 0x33, 16);         
		this->dctCoeffNextRunTable.add(1, 0x26, 17);         
		this->dctCoeffNextRunTable.add(1, 0x27, 17);         
		this->dctCoeffNextRunTable.add(1, 0x24, 17);         
		this->dctCoeffNextRunTable.add(1, 0x25, 17);         
		this->dctCoeffNextRunTable.add(1, 0x22, 17);         
		this->dctCoeffNextRunTable.add(1, 0x23, 17);         
		this->dctCoeffNextRunTable.add(1, 0x20, 17);         
		this->dctCoeffNextRunTable.add(1, 0x21, 17);         
		this->dctCoeffNextRunTable.add(6, 0x28, 17);         
		this->dctCoeffNextRunTable.add(6, 0x29, 17);         
		this->dctCoeffNextRunTable.add(11, 0x34, 17);        
		this->dctCoeffNextRunTable.add(11, 0x35, 17);        
		this->dctCoeffNextRunTable.add(12, 0x32, 17);        
		this->dctCoeffNextRunTable.add(12, 0x33, 17);        
		this->dctCoeffNextRunTable.add(13, 0x30, 17);        
		this->dctCoeffNextRunTable.add(13, 0x31, 17);        
		this->dctCoeffNextRunTable.add(14, 0x2E, 17);        
		this->dctCoeffNextRunTable.add(14, 0x2F, 17);        
		this->dctCoeffNextRunTable.add(15, 0x2C, 17);        
		this->dctCoeffNextRunTable.add(15, 0x2D, 17);        
		this->dctCoeffNextRunTable.add(16, 0x2A, 17);        
		this->dctCoeffNextRunTable.add(16, 0x2B, 17);        
		this->dctCoeffNextRunTable.add(27, 0x3E, 17);        
		this->dctCoeffNextRunTable.add(27, 0x3F, 17);        
		this->dctCoeffNextRunTable.add(28, 0x3C, 17);        
		this->dctCoeffNextRunTable.add(28, 0x3D, 17);        
		this->dctCoeffNextRunTable.add(29, 0x3A, 17);        
		this->dctCoeffNextRunTable.add(29, 0x3B, 17);        
		this->dctCoeffNextRunTable.add(30, 0x38, 17);        
		this->dctCoeffNextRunTable.add(30, 0x39, 17);        
		this->dctCoeffNextRunTable.add(31, 0x36, 17);        
		this->dctCoeffNextRunTable.add(31, 0x37, 17);        
		this->dctCoeffNextRunTable.initialize();             
		
		this->dctCoeffNextLevelTable.add(1, 0x6, 3);
		this->dctCoeffNextLevelTable.add(-1, 0x7, 3);
		this->dctCoeffNextLevelTable.add(1, 0x6, 4);
		this->dctCoeffNextLevelTable.add(-1, 0x7, 4);
		this->dctCoeffNextLevelTable.add(2, 0x8, 5);
		this->dctCoeffNextLevelTable.add(-2, 0x9, 5);
		this->dctCoeffNextLevelTable.add(1, 0xA, 5);
		this->dctCoeffNextLevelTable.add(-1, 0xB, 5);
		this->dctCoeffNextLevelTable.add(3, 0xA, 6);
		this->dctCoeffNextLevelTable.add(-3, 0xB, 6);
		this->dctCoeffNextLevelTable.add(1, 0xE, 6);
		this->dctCoeffNextLevelTable.add(-1, 0xF, 6);
		this->dctCoeffNextLevelTable.add(1, 0xC, 6);
		this->dctCoeffNextLevelTable.add(-1, 0xD, 6);
		this->dctCoeffNextLevelTable.add(2, 0xC, 7);
		this->dctCoeffNextLevelTable.add(-2, 0xD, 7);
		this->dctCoeffNextLevelTable.add(1, 0xE, 7);
		this->dctCoeffNextLevelTable.add(-1, 0xF, 7);
		this->dctCoeffNextLevelTable.add(1, 0xA, 7);
		this->dctCoeffNextLevelTable.add(-1, 0xB, 7);
		this->dctCoeffNextLevelTable.add(1, 0x8, 7);
		this->dctCoeffNextLevelTable.add(-1, 0x9, 7);
		this->dctCoeffNextLevelTable.add(4, 0xC, 8);
		this->dctCoeffNextLevelTable.add(-4, 0xD, 8);
		this->dctCoeffNextLevelTable.add(2, 0x8, 8);
		this->dctCoeffNextLevelTable.add(-2, 0x9, 8);
		this->dctCoeffNextLevelTable.add(1, 0xE, 8);
		this->dctCoeffNextLevelTable.add(-1, 0xF, 8);
		this->dctCoeffNextLevelTable.add(1, 0xA, 8);
		this->dctCoeffNextLevelTable.add(-1, 0xB, 8);
		this->dctCoeffNextLevelTable.add(0, 0x1, 6);
		this->dctCoeffNextLevelTable.add(5, 0x4C, 9);
		this->dctCoeffNextLevelTable.add(-5, 0x4D, 9);
		this->dctCoeffNextLevelTable.add(6, 0x42, 9);
		this->dctCoeffNextLevelTable.add(-6, 0x43, 9);
		this->dctCoeffNextLevelTable.add(3, 0x4A, 9);
		this->dctCoeffNextLevelTable.add(-3, 0x4B, 9);
		this->dctCoeffNextLevelTable.add(2, 0x48, 9);
		this->dctCoeffNextLevelTable.add(-2, 0x49, 9);
		this->dctCoeffNextLevelTable.add(1, 0x4E, 9);
		this->dctCoeffNextLevelTable.add(-1, 0x4F, 9);
		this->dctCoeffNextLevelTable.add(1, 0x46, 9);
		this->dctCoeffNextLevelTable.add(-1, 0x47, 9);
		this->dctCoeffNextLevelTable.add(1, 0x44, 9);
		this->dctCoeffNextLevelTable.add(-1, 0x45, 9);
		this->dctCoeffNextLevelTable.add(1, 0x40, 9);
		this->dctCoeffNextLevelTable.add(-1, 0x41, 9);
		this->dctCoeffNextLevelTable.add(7, 0x14, 11);
		this->dctCoeffNextLevelTable.add(-7, 0x15, 11);
		this->dctCoeffNextLevelTable.add(4, 0x18, 11);
		this->dctCoeffNextLevelTable.add(-4, 0x19, 11);
		this->dctCoeffNextLevelTable.add(3, 0x16, 11);
		this->dctCoeffNextLevelTable.add(-3, 0x17, 11);
		this->dctCoeffNextLevelTable.add(2, 0x1E, 11);
		this->dctCoeffNextLevelTable.add(-2, 0x1F, 11);
		this->dctCoeffNextLevelTable.add(2, 0x12, 11);
		this->dctCoeffNextLevelTable.add(-2, 0x13, 11);
		this->dctCoeffNextLevelTable.add(1, 0x1C, 11);
		this->dctCoeffNextLevelTable.add(-1, 0x1D, 11);
		this->dctCoeffNextLevelTable.add(1, 0x1A, 11);
		this->dctCoeffNextLevelTable.add(-1, 0x1B, 11);
		this->dctCoeffNextLevelTable.add(1, 0x10, 11);
		this->dctCoeffNextLevelTable.add(-1, 0x11, 11);
		this->dctCoeffNextLevelTable.add(8, 0x3A, 13);
		this->dctCoeffNextLevelTable.add(-8, 0x3B, 13);
		this->dctCoeffNextLevelTable.add(9, 0x30, 13);      
		this->dctCoeffNextLevelTable.add(-9, 0x31, 13);     
		this->dctCoeffNextLevelTable.add(10, 0x26, 13);
		this->dctCoeffNextLevelTable.add(-10, 0x27, 13);
		this->dctCoeffNextLevelTable.add(11, 0x20, 13);
		this->dctCoeffNextLevelTable.add(-11, 0x21, 13);
		this->dctCoeffNextLevelTable.add(5, 0x36, 13);
		this->dctCoeffNextLevelTable.add(-5, 0x37, 13);
		this->dctCoeffNextLevelTable.add(4, 0x28, 13);
		this->dctCoeffNextLevelTable.add(-4, 0x29, 13);
		this->dctCoeffNextLevelTable.add(3, 0x38, 13);
		this->dctCoeffNextLevelTable.add(-3, 0x39, 13);
		this->dctCoeffNextLevelTable.add(3, 0x24, 13);
		this->dctCoeffNextLevelTable.add(-3, 0x25, 13);
		this->dctCoeffNextLevelTable.add(2, 0x3C, 13);
		this->dctCoeffNextLevelTable.add(-2, 0x3D, 13);
		this->dctCoeffNextLevelTable.add(2, 0x2A, 13);
		this->dctCoeffNextLevelTable.add(-2, 0x2B, 13);
		this->dctCoeffNextLevelTable.add(2, 0x22, 13);
		this->dctCoeffNextLevelTable.add(-2, 0x23, 13);
		this->dctCoeffNextLevelTable.add(1, 0x3E, 13);
		this->dctCoeffNextLevelTable.add(-1, 0x3F, 13);
		this->dctCoeffNextLevelTable.add(1, 0x34, 13);
		this->dctCoeffNextLevelTable.add(-1, 0x35, 13);
		this->dctCoeffNextLevelTable.add(1, 0x32, 13);
		this->dctCoeffNextLevelTable.add(-1, 0x33, 13);
		this->dctCoeffNextLevelTable.add(1, 0x2E, 13);
		this->dctCoeffNextLevelTable.add(-1, 0x2F, 13);
		this->dctCoeffNextLevelTable.add(1, 0x2C, 13);
		this->dctCoeffNextLevelTable.add(-1, 0x2D, 13);
		this->dctCoeffNextLevelTable.add(12, 0x34, 14);
		this->dctCoeffNextLevelTable.add(-12, 0x35, 14);
		this->dctCoeffNextLevelTable.add(13, 0x32, 14);
		this->dctCoeffNextLevelTable.add(-13, 0x33, 14);
		this->dctCoeffNextLevelTable.add(14, 0x30, 14);
		this->dctCoeffNextLevelTable.add(-14, 0x31, 14);
		this->dctCoeffNextLevelTable.add(15, 0x2E, 14);
		this->dctCoeffNextLevelTable.add(-15, 0x2F, 14);
		this->dctCoeffNextLevelTable.add(6, 0x2C, 14);
		this->dctCoeffNextLevelTable.add(-6, 0x2D, 14);
		this->dctCoeffNextLevelTable.add(7, 0x2A, 14);
		this->dctCoeffNextLevelTable.add(-7, 0x2B, 14);
		this->dctCoeffNextLevelTable.add(5, 0x28, 14);
		this->dctCoeffNextLevelTable.add(-5, 0x29, 14);
		this->dctCoeffNextLevelTable.add(4, 0x26, 14);
		this->dctCoeffNextLevelTable.add(-4, 0x27, 14);
		this->dctCoeffNextLevelTable.add(3, 0x24, 14);
		this->dctCoeffNextLevelTable.add(-3, 0x25, 14);
		this->dctCoeffNextLevelTable.add(2, 0x22, 14);
		this->dctCoeffNextLevelTable.add(-2, 0x23, 14);
		this->dctCoeffNextLevelTable.add(2, 0x20, 14);
		this->dctCoeffNextLevelTable.add(-2, 0x21, 14);
		this->dctCoeffNextLevelTable.add(1, 0x3E, 14);
		this->dctCoeffNextLevelTable.add(-1, 0x3F, 14);
		this->dctCoeffNextLevelTable.add(1, 0x3C, 14);
		this->dctCoeffNextLevelTable.add(-1, 0x3D, 14);
		this->dctCoeffNextLevelTable.add(1, 0x3A, 14);
		this->dctCoeffNextLevelTable.add(-1, 0x3B, 14);
		this->dctCoeffNextLevelTable.add(1, 0x38, 14);
		this->dctCoeffNextLevelTable.add(-1, 0x39, 14);
		this->dctCoeffNextLevelTable.add(1, 0x36, 14);
		this->dctCoeffNextLevelTable.add(-1, 0x37, 14);
		this->dctCoeffNextLevelTable.add(16, 0x3E, 15);
		this->dctCoeffNextLevelTable.add(-16, 0x3F, 15);
		this->dctCoeffNextLevelTable.add(17, 0x3C, 15);
		this->dctCoeffNextLevelTable.add(-17, 0x3D, 15);
		this->dctCoeffNextLevelTable.add(18, 0x3A, 15);
		this->dctCoeffNextLevelTable.add(-18, 0x3B, 15);
		this->dctCoeffNextLevelTable.add(19, 0x38, 15);
		this->dctCoeffNextLevelTable.add(-19, 0x39, 15);
		this->dctCoeffNextLevelTable.add(20, 0x36, 15);
		this->dctCoeffNextLevelTable.add(-20, 0x37, 15);
		this->dctCoeffNextLevelTable.add(21, 0x34, 15);
		this->dctCoeffNextLevelTable.add(-21, 0x35, 15);
		this->dctCoeffNextLevelTable.add(22, 0x32, 15);
		this->dctCoeffNextLevelTable.add(-22, 0x33, 15);
		this->dctCoeffNextLevelTable.add(23, 0x30, 15);
		this->dctCoeffNextLevelTable.add(-23, 0x31, 15);
		this->dctCoeffNextLevelTable.add(24, 0x2E, 15);
		this->dctCoeffNextLevelTable.add(-24, 0x2F, 15);
		this->dctCoeffNextLevelTable.add(25, 0x2C, 15);
		this->dctCoeffNextLevelTable.add(-25, 0x2D, 15);
		this->dctCoeffNextLevelTable.add(26, 0x2A, 15);
		this->dctCoeffNextLevelTable.add(-26, 0x2B, 15);
		this->dctCoeffNextLevelTable.add(27, 0x28, 15);
		this->dctCoeffNextLevelTable.add(-27, 0x29, 15);
		this->dctCoeffNextLevelTable.add(28, 0x26, 15);
		this->dctCoeffNextLevelTable.add(-28, 0x27, 15);
		this->dctCoeffNextLevelTable.add(29, 0x24, 15);
		this->dctCoeffNextLevelTable.add(-29, 0x25, 15);
		this->dctCoeffNextLevelTable.add(30, 0x22, 15);
		this->dctCoeffNextLevelTable.add(-30, 0x23, 15);
		this->dctCoeffNextLevelTable.add(31, 0x20, 15);
		this->dctCoeffNextLevelTable.add(-31, 0x21, 15);
		this->dctCoeffNextLevelTable.add(32, 0x30, 16);
		this->dctCoeffNextLevelTable.add(-32, 0x31, 16);
		this->dctCoeffNextLevelTable.add(33, 0x2E, 16);
		this->dctCoeffNextLevelTable.add(-33, 0x2F, 16);
		this->dctCoeffNextLevelTable.add(34, 0x2C, 16);
		this->dctCoeffNextLevelTable.add(-34, 0x2D, 16);
		this->dctCoeffNextLevelTable.add(35, 0x2A, 16);
		this->dctCoeffNextLevelTable.add(-35, 0x2B, 16);
		this->dctCoeffNextLevelTable.add(36, 0x28, 16);
		this->dctCoeffNextLevelTable.add(-36, 0x29, 16);
		this->dctCoeffNextLevelTable.add(37, 0x26, 16);
		this->dctCoeffNextLevelTable.add(-37, 0x27, 16);
		this->dctCoeffNextLevelTable.add(38, 0x24, 16);
		this->dctCoeffNextLevelTable.add(-38, 0x25, 16);
		this->dctCoeffNextLevelTable.add(39, 0x22, 16);
		this->dctCoeffNextLevelTable.add(-39, 0x23, 16);
		this->dctCoeffNextLevelTable.add(40, 0x20, 16);
		this->dctCoeffNextLevelTable.add(-40, 0x21, 16);
		this->dctCoeffNextLevelTable.add(8, 0x3E, 16);
		this->dctCoeffNextLevelTable.add(-8, 0x3F, 16);
		this->dctCoeffNextLevelTable.add(9, 0x3C, 16);
		this->dctCoeffNextLevelTable.add(-9, 0x3D, 16);
		this->dctCoeffNextLevelTable.add(10, 0x3A, 16);
		this->dctCoeffNextLevelTable.add(-10, 0x3B, 16);
		this->dctCoeffNextLevelTable.add(11, 0x38, 16);
		this->dctCoeffNextLevelTable.add(-11, 0x39, 16);
		this->dctCoeffNextLevelTable.add(12, 0x36, 16);
		this->dctCoeffNextLevelTable.add(-12, 0x37, 16);
		this->dctCoeffNextLevelTable.add(13, 0x34, 16);
		this->dctCoeffNextLevelTable.add(-13, 0x35, 16);
		this->dctCoeffNextLevelTable.add(14, 0x32, 16);
		this->dctCoeffNextLevelTable.add(-14, 0x33, 16);
		this->dctCoeffNextLevelTable.add(15, 0x26, 17);
		this->dctCoeffNextLevelTable.add(-15, 0x27, 17);
		this->dctCoeffNextLevelTable.add(16, 0x24, 17);
		this->dctCoeffNextLevelTable.add(-16, 0x25, 17);
		this->dctCoeffNextLevelTable.add(17, 0x22, 17);
		this->dctCoeffNextLevelTable.add(-17, 0x23, 17);
		this->dctCoeffNextLevelTable.add(18, 0x20, 17);
		this->dctCoeffNextLevelTable.add(-18, 0x21, 17);
		this->dctCoeffNextLevelTable.add(3, 0x28, 17);
		this->dctCoeffNextLevelTable.add(-3, 0x29, 17);
		this->dctCoeffNextLevelTable.add(2, 0x34, 17);
		this->dctCoeffNextLevelTable.add(-2, 0x35, 17);
		this->dctCoeffNextLevelTable.add(2, 0x32, 17);
		this->dctCoeffNextLevelTable.add(-2, 0x33, 17);
		this->dctCoeffNextLevelTable.add(2, 0x30, 17);
		this->dctCoeffNextLevelTable.add(-2, 0x31, 17);
		this->dctCoeffNextLevelTable.add(2, 0x2E, 17);
		this->dctCoeffNextLevelTable.add(-2, 0x2F, 17);
		this->dctCoeffNextLevelTable.add(2, 0x2C, 17);
		this->dctCoeffNextLevelTable.add(-2, 0x2D, 17);
		this->dctCoeffNextLevelTable.add(2, 0x2A, 17);
		this->dctCoeffNextLevelTable.add(-2, 0x2B, 17);
		this->dctCoeffNextLevelTable.add(1, 0x3E, 17);
		this->dctCoeffNextLevelTable.add(-1, 0x3F, 17);
		this->dctCoeffNextLevelTable.add(1, 0x3C, 17);
		this->dctCoeffNextLevelTable.add(-1, 0x3D, 17);
		this->dctCoeffNextLevelTable.add(1, 0x3A, 17);
		this->dctCoeffNextLevelTable.add(-1, 0x3B, 17);
		this->dctCoeffNextLevelTable.add(1, 0x38, 17);
		this->dctCoeffNextLevelTable.add(-1, 0x39, 17);
        this->dctCoeffNextLevelTable.add(1, 0x36, 17);
        this->dctCoeffNextLevelTable.add(-1, 0x37, 17);
        this->dctCoeffNextLevelTable.initialize();
		
		int zz[64];
		for(int i = 0; i < 64; i ++){
			zz[i] = i;
		}
		this->scan.zigzag(zz);
	}
	
	
    
    FILE *MPEGFile;
    char *BMPDirectoryName;
	void decode(char *MPEGFileName, char *BMPDirName){
		this->MPEGFile = fopen(MPEGFileName, "rb");
		this->BMPDirectoryName = BMPDirName;
		videoSequence();
	}
	
	int fileStartBit;
	vector< unsigned char > byteBuffer;
	unsigned int nextBits(int nBits, bool filePointerFixed = false){
		unsigned int bits = 0;
		for(int i = 0; i < nBits; i ++){
			if(this->fileStartBit / 8 >= this->byteBuffer.size()){
				unsigned char buf[2] = {0};
				fread(buf, sizeof(unsigned char), 1, this->MPEGFile);
				this->byteBuffer.push_back(buf[0]);
			}
			bits = (bits << 1) | ((this->byteBuffer[this->fileStartBit >> 3] >> (7 - (this->fileStartBit & 7))) & 1);
			this->fileStartBit ++;
		}
		if(filePointerFixed){
			 this->fileStartBit -= nBits;
		}
		return bits;
	}
	
	void initializeByteBuffer(){
		this->fileStartBit = 0;
		this->byteBuffer.clear();
	}
	
	bool byteAligned(){
		return this->fileStartBit % 8 == 0;
	}
	
	void nextStartCode(){
		while(!byteAligned()){
			nextBits(1);
		}
		int unusedByteBufferCount = this->fileStartBit >> 3;
		for(int i = 0; i < unusedByteBufferCount; i ++){
			this->byteBuffer.erase(this->byteBuffer.begin());
		}
		this->fileStartBit = 0;
		while(nextBits(24, true) != 0x1){
			nextBits(8);
		}
	}

	unsigned int totalPictureIndex;
	void videoSequence(){
		initializeByteBuffer();
		nextStartCode();
		this->totalPictureIndex = 0;
		
		do{
			sequenceHeader();
			int index = 0;
			do{
				groupOfPictures(index);
				index ++;
			}while(nextBits(32, true) == GROUP_START_CODE);
			
		}while(nextBits(32, true) == SEQUENCE_HEADER_CODE);
		unsigned int sequenceEndCode = nextBits(32);
		//printf("\nsequenceEndCode: %x\n", sequenceEndCode);
		
	}
	
	unsigned int horizontalSize;
	unsigned int verticalSize;
	unsigned int mbWidth;
	unsigned int mbHeight;
	unsigned int pelAspectRatio;
	unsigned int pictureRate;
	unsigned int bitRate;
	unsigned int vbvBufferSize;
	unsigned int B;
	unsigned int constrainedParameterFlag;
	Block intraQuantizerTable;
	Block nonIntraQuantizerTable;
	vector< vector<Pixel> > image;
	vector< vector<Pixel> > lastImage;
	vector< vector<Pixel> > nextImage;
	void sequenceHeader(){
		unsigned int sequenceHeaderCode = nextBits(32);
		//printf("\nsequenceHeaderCode: %x\n", sequenceHeaderCode);
		
		this->horizontalSize = nextBits(12);
		this->mbWidth = (this->horizontalSize + 15 ) >> 4;
		//printf("horizontalSize: %d, mbWidth: %d\n", this->horizontalSize, this->mbWidth);
		
		this->verticalSize = nextBits(12);
		this->mbHeight = (this->verticalSize + 15 ) >> 4;
		//printf("verticalSize: %d, mbHeight: %d\n", this->verticalSize, this->mbHeight);
		
		this->pelAspectRatio = nextBits(4);
		//printf("pelAspectRatio: %x\n", this->pelAspectRatio); // Todo: Relevant table
		
		this->pictureRate = nextBits(4);
		//printf("pictureRate: %x\n", this->pictureRate); // Todo: Relevant table
		
		this->bitRate = nextBits(18);
		//printf("bitRate: %x\n", this->bitRate);
		
		unsigned int markerBit = nextBits(1);
		//printf("markerBit: %x\n", markerBit);
		
		this->vbvBufferSize = nextBits(10);
		this->B = this->vbvBufferSize << 14;
		//printf("vbvBufferSize: %x\n", this->vbvBufferSize);
		
		this->constrainedParameterFlag = nextBits(1);
		//printf("constrainedParameterFlag: %x\n", this->constrainedParameterFlag);
		
		unsigned int loadIntraQuantizerTable = nextBits(1);
		//printf("loadIntraQuantizerTable: %d\n", loadIntraQuantizerTable);
		
		int intraQTBuf[70];
		if(loadIntraQuantizerTable){
			for(int i = 0; i < 63; i ++){
				intraQTBuf[i] = (int)nextBits(8);
			}
		}
		else{
			intraQTBuf[0] = 8;
			intraQTBuf[1] = 16;
			intraQTBuf[2] = 16;
			intraQTBuf[3] = 19;
			intraQTBuf[4] = 16;
			intraQTBuf[5] = 19;
			intraQTBuf[6] = 22;
			intraQTBuf[7] = 22;
			intraQTBuf[8] = 22;
			intraQTBuf[9] = 22;
			intraQTBuf[10] = 22;
			intraQTBuf[11] = 22;
			intraQTBuf[12] = 26;
			intraQTBuf[13] = 24;
			intraQTBuf[14] = 26;
			intraQTBuf[15] = 27;
			intraQTBuf[16] = 27;
			intraQTBuf[17] = 27;
			intraQTBuf[18] = 26;
			intraQTBuf[19] = 26;
			intraQTBuf[20] = 26;
			intraQTBuf[21] = 26;
			intraQTBuf[22] = 27;
			intraQTBuf[23] = 27;
			intraQTBuf[24] = 27;
			intraQTBuf[25] = 29;
			intraQTBuf[26] = 29;
			intraQTBuf[27] = 29;
			intraQTBuf[28] = 34;
			intraQTBuf[29] = 34;
			intraQTBuf[30] = 34;
			intraQTBuf[31] = 29;
			intraQTBuf[32] = 29;
			intraQTBuf[33] = 29;
			intraQTBuf[34] = 27;
			intraQTBuf[35] = 27;
			intraQTBuf[36] = 29;
			intraQTBuf[37] = 29;
			intraQTBuf[38] = 32;
			intraQTBuf[39] = 32;
			intraQTBuf[40] = 34;
			intraQTBuf[41] = 34;
			intraQTBuf[42] = 37;
			intraQTBuf[43] = 38;
			intraQTBuf[44] = 37;
			intraQTBuf[45] = 35;
			intraQTBuf[46] = 35;
			intraQTBuf[47] = 34;
			intraQTBuf[48] = 35;
			intraQTBuf[49] = 38;
			intraQTBuf[50] = 38;
			intraQTBuf[51] = 40;
			intraQTBuf[52] = 40;
			intraQTBuf[53] = 40;
			intraQTBuf[54] = 48;
			intraQTBuf[55] = 48;
			intraQTBuf[56] = 46;
			intraQTBuf[57] = 46;
			intraQTBuf[58] = 56;
			intraQTBuf[59] = 56;
			intraQTBuf[60] = 58;
			intraQTBuf[61] = 69;
			intraQTBuf[62] = 69;
			intraQTBuf[63] = 83;
		}
		this->intraQuantizerTable.zigzag(intraQTBuf);
		//this->intraQuantizerTable.print();
		
		unsigned int loadNonIntraQuantizerTable = nextBits(1);
		//printf("loadNonIntraQuantizerTable: %d\n", loadIntraQuantizerTable);
		
		int nonIntraQTBuf[70];
		if(loadNonIntraQuantizerTable){
			for(int i = 0; i < 63; i ++){
				nonIntraQTBuf[i] = (int)nextBits(8);
			}
		}
		else{
			for(int i = 0; i < 64; i ++){
				nonIntraQTBuf[i] = 16;
			}
		}
		this->nonIntraQuantizerTable.zigzag(nonIntraQTBuf);
		//this->nonIntraQuantizerTable.print();
		
		nextStartCode();
		if(nextBits(32, true) == EXTENSION_START_CODE){
			nextBits(32);
			while(nextBits(24, true) != 0x1){
				nextBits(8);
			}
			nextStartCode();
		}
		if(nextBits(32, true) == USER_DATA_START_CODE){
			nextBits(32);
			while(nextBits(24, true) != 0x1){
				nextBits(8);
			}
			nextStartCode();
		}
		
		this->image.clear();
		this->lastImage.clear();
		this->nextImage.clear();
		for(int y = 0; y < this->verticalSize; y ++){
			vector<Pixel> imageRow(this->horizontalSize);
			this->image.push_back(imageRow);
			
			vector<Pixel> lastImageRow(this->horizontalSize);
			this->lastImage.push_back(lastImageRow);
			
			vector<Pixel> nextImageRow(this->horizontalSize);
			this->nextImage.push_back(nextImageRow);
		}
	}
	
	unsigned int dropFrameFlag;
	unsigned int timeCodeHours;
	unsigned int timeCodeMinutes;
	unsigned int timeCodeSeconds;
	unsigned int timeCodePictures;
	unsigned int closedGop;
	unsigned int brokenLink;
	void groupOfPictures(int index){
		unsigned int groupStartCode = nextBits(32);
		//printf("\ngroupStartCode %d: %x\n", index, groupStartCode);
		
		this->dropFrameFlag = nextBits(1);
		//printf("dropFrameFlag: %x\n", this->dropFrameFlag);
		
		this->timeCodeHours = nextBits(5);
		//printf("timeCodeHours: %x\n", this->timeCodeHours);
		
		this->timeCodeMinutes = nextBits(6);
		//printf("timeCodeMinutes: %x\n", this->timeCodeMinutes);
		
		unsigned int markerBit = nextBits(1);
		//printf("markerBit: %x\n", markerBit);
		
		this->timeCodeSeconds = nextBits(6);
		//printf("timeCodeSeconds: %x\n", this->timeCodeSeconds);
		
		this->timeCodePictures = nextBits(6);
		//printf("timeCodePictures: %x\n", this->timeCodePictures);
		
		this->closedGop = nextBits(1);
		//printf("closedGop: %x\n", this->closedGop);
		
		this->brokenLink = nextBits(1);
		//printf("brokenLink: %x\n", this->brokenLink);
		
		nextStartCode();
		if(nextBits(32, true) == EXTENSION_START_CODE){
			nextBits(32);
			while(nextBits(24, true) != 0x1){
				nextBits(8);
			}
			nextStartCode();
		}
		if(nextBits(32, true) == USER_DATA_START_CODE){
			nextBits(32);
			while(nextBits(24, true) != 0x1){
				nextBits(8);
			}
			nextStartCode();
		}
		
		int indexPic = 0;
		do{
			picture(indexPic);
			indexPic ++;
			
		}while(nextBits(32, true) == PICTURE_START_CODE);
		
	}
	
	unsigned int temporalReference;
	unsigned int pictureCodingType;
	unsigned int vbvDelay;
	unsigned int fullPelForwardVector;
	unsigned int forwardFCode;
	unsigned int fullPelBackwardVector;
	unsigned int backwardFCode;
	int forwardRSize;
	int forwardF;
	int backwardRSize;
	int backwardF;
	unsigned int lastPictureCodingType;
	unsigned int nextPictureCodingType;
	void picture(int index){
		unsigned int pictureStartCode = nextBits(32);
		//printf("\npictureStartCode %d: %x\n", index, pictureStartCode);
		
		this->temporalReference = nextBits(10);
		//printf("temporalReference: %x\n", this->temporalReference);
		
		this->pictureCodingType = nextBits(3);
		//printf("pictureCodingType: %x\n", this->pictureCodingType);
	
		this->vbvDelay = nextBits(16);
		//printf("vbvDelay: %x\n", this->vbvDelay);
		
		if(this->pictureCodingType == P_FRAME || this->pictureCodingType == B_FRAME){
			this->fullPelForwardVector = nextBits(1);
			this->forwardFCode = nextBits(3);
			this->forwardRSize = this->forwardFCode - 1;
			this->forwardF = 1 << this->forwardRSize;
		}
		
		if(this->pictureCodingType == B_FRAME){
			this->fullPelBackwardVector = nextBits(1);
			this->backwardFCode = nextBits(3);
			this->backwardRSize = this->backwardFCode - 1;
			this->backwardF = 1 << this->backwardRSize;
		}
		
		unsigned int extraBitPicture;
		unsigned int extraInformationPicture;
		while(nextBits(1, true) == 0x1){
			extraBitPicture = nextBits(1);
			extraInformationPicture = nextBits(8);
		}
		extraBitPicture = nextBits(1);
		
		nextStartCode();
		if(nextBits(32, true) == EXTENSION_START_CODE){
			nextBits(32);
			while(nextBits(24, true) != 0x1){
				nextBits(8);
			}
			nextStartCode();
		}
		if(nextBits(32, true) == USER_DATA_START_CODE){
			nextBits(32);
			while(nextBits(24, true) != 0x1){
				nextBits(8);
			}
			nextStartCode();
		}
		
		if(this->pictureCodingType == I_FRAME || this->pictureCodingType == P_FRAME){
			this->lastImage = this->nextImage;
			imageRGBTransform(this->lastImage);
			outputBMP(this->lastImage);
		}

		// Decode picture
		do{
			slice();
		}while(isSliceStartCode());
		
		
		if(this->pictureCodingType == I_FRAME || this->pictureCodingType == P_FRAME){
			this->lastImage = this->nextImage;
			this->nextImage = this->image;
		}
		else if(this->pictureCodingType == B_FRAME){
			imageRGBTransform(this->image);
			outputBMP(this->image);
		}
		
	}
	
	bool isSliceStartCode(){
		unsigned int nextCode = nextBits(32, true);
		return nextCode >= 0x101 && nextCode <= 0x1AF;
	}
	
	void imageRGBTransform(vector< vector<Pixel> > &img){
		for(int y = 0; y < this->verticalSize; y ++){
			for(int x = 0; x < this->horizontalSize; x ++){
				img[y][x].RGBTrans();
			}
		}
	}
	
	void outputBMP(vector< vector<Pixel> > &img){
		char bmpFileName[100];
		sprintf(bmpFileName, "%s/%d.bmp", this->BMPDirectoryName, this->totalPictureIndex);
		FILE *bmpFile = fopen(bmpFileName, "wb");
		this->totalPictureIndex ++;
		
		// "BM"
		unsigned char buffer[10];
		buffer[0] = 0x42;
		buffer[1] = 0x4D;
		fwrite(buffer, sizeof(char), 2, bmpFile);
		
		// Image size
		int width = ((this->horizontalSize * 24 + 31) / 32) * 4;
		int space = width * this->verticalSize + 0x36;
		fseek(bmpFile, 0x2, SEEK_SET);
		buffer[0] = space & 0xFF;
		buffer[1] = (space >> 8) & 0xFF;
		buffer[2] = (space >> 16) & 0xFF;
		buffer[3] = (space >> 24) & 0xFF;
		fwrite(buffer, sizeof(char), 4, bmpFile);
		
		fseek(bmpFile, 0xA, SEEK_SET);
		buffer[0] = 0x36;
		fwrite(buffer, sizeof(char), 1, bmpFile);
		
		fseek(bmpFile, 0xE, SEEK_SET);
		buffer[0] = 0x28;
		fwrite(buffer, sizeof(char), 1, bmpFile);
		
		// Image width
		fseek(bmpFile, 0x12, SEEK_SET);
		buffer[0] = this->horizontalSize & 0xFF;
		buffer[1] = (this->horizontalSize >> 8) & 0xFF;
		buffer[2] = (this->horizontalSize >> 16) & 0xFF;
		buffer[3] = (this->horizontalSize >> 24) & 0xFF;
		fwrite(buffer, sizeof(char), 4, bmpFile);
		
		// Image height
		fseek(bmpFile, 0x16, SEEK_SET);
		buffer[0] = this->verticalSize & 0xFF;
		buffer[1] = (this->verticalSize >> 8) & 0xFF;
		buffer[2] = (this->verticalSize >> 16) & 0xFF;
		buffer[3] = (this->verticalSize >> 24) & 0xFF;
		fwrite(buffer, sizeof(char), 4, bmpFile);
		
		fseek(bmpFile, 0x1A, SEEK_SET);
		buffer[0] = 1;
		fwrite(buffer, sizeof(char), 1, bmpFile);
		
		fseek(bmpFile, 0x1C, SEEK_SET);
		buffer[0] = 24;
		fwrite(buffer, sizeof(char), 1, bmpFile);
		
		fseek(bmpFile, 0x36, SEEK_SET);
		for(int v = this->verticalSize - 1; v >= 0; v --){
			int lineIndex = 0;
			for(int h = 0; h < this->horizontalSize; h ++){
				lineIndex ++;
				int color;
				color = img[v][h].B;
				buffer[0] = (unsigned char) color;
				color = img[v][h].G;
				buffer[1] = (unsigned char) color;
				color = img[v][h].R;
				buffer[2] = (unsigned char) color;
				fwrite(buffer, sizeof(char), 3, bmpFile);
			}
			while(lineIndex % 4 > 0){
				buffer[0] = 0;
				fwrite(buffer, sizeof(char), 1, bmpFile);
				lineIndex ++;
			}
		}
		
		fclose(bmpFile);
	}

	unsigned int sliceVerticalPosition;
	unsigned int quantizerScale;
	unsigned int previousMacroblockAddress;
	int dctDcYPast;
	int dctDcCbPast;
	int dctDcCrPast;
	int pastIntraAddress;
	unsigned int macroblockIntra;
	
	int reconRightFor;
	int reconDownFor;
	int reconRightForPrev;
	int reconDownForPrev;
	int reconRightBack;
	int reconDownBack;
	int reconRightBackPrev;
	int reconDownBackPrev;
	void slice(){
		unsigned int sliceStartCode = nextBits(32);
		//printf("\nsliceStartCode: %x\n", sliceStartCode);
		
		this->sliceVerticalPosition = sliceStartCode & 0xFF;
		//printf("sliceVerticalPosition: %x\n", sliceVerticalPosition);
		
		this->quantizerScale = nextBits(5);
		//printf("quantizerScale: %x\n", this->quantizerScale);
		
		unsigned int extraBitSlice;
		unsigned int extraInformationSlice;
		while(nextBits(1, true) == 0x1){
			extraBitSlice = nextBits(1);
			extraInformationSlice = nextBits(8);
		}
		extraBitSlice = nextBits(1);
		
		this->previousMacroblockAddress = (this->sliceVerticalPosition - 1) * this->mbWidth - 1;
		//printf("slice start previousMacroblockAddress: %d\n", this->previousMacroblockAddress);
		
		this->dctDcYPast = 1024;
		this->dctDcCbPast = 1024;
		this->dctDcCrPast = 1024;
		this->pastIntraAddress = -2;
		
		this->reconRightForPrev = 0;
		this->reconDownForPrev = 0;
		this->reconRightBackPrev = 0;
		this->reconDownBackPrev = 0;
		
		int index = 0;
		do{
			macroblock(index);
			if(this->macroblockIntra){
				this->pastIntraAddress = this->macroblockAddress;
			}
			index ++;
		}while(nextBits(23, true) != 0x0);
		nextStartCode();
		//printf("\nnext start code: %x\n", nextBits(32, true));
	}

	unsigned int macroblockAddressIncrement;
	unsigned int macroblockAddress;
	unsigned int mbRow;
	unsigned int mbColumn;
	unsigned int macroblockQuant;
	unsigned int macroblockMotionForward;
	unsigned int macroblockMotionBackward;
	unsigned int macroblockPattern;
	
	int motionHorizontalForwardCode;
	int motionHorizontalForwardR;
	int motionVerticalForwardCode;
	int motionVerticalForwardR;
	int motionHorizontalBackwardCode;
	int motionHorizontalBackwardR;
	int motionVerticalBackwardCode;
	int motionVerticalBackwardR;
	int cbp;
	unsigned int patternCode[6];

	
	int complementHorizontalForwardR;
	int complementHorizontalBackwardR;
	int complementVerticalForwardR;
	int complementVerticalBackwardR;
	int rightLittle;
	int rightBig;
	int downLittle;
	int downBig;
	int rightFor;
	int downFor;
	int rightHalfFor;
	int downHalfFor;
	int rightBack;
	int downBack;
	int rightHalfBack;
	int downHalfBack;
	void macroblock(int index){
		//printf("\nmacroblock %d\n", index);
		
		unsigned int macroblockStuffing;
		while(nextBits(11, true) == 0xF){
			macroblockStuffing = nextBits(11);
		}
		
		unsigned int macroblockEscape;
		int counter = 0;
		while(nextBits(11, true) == 0x8){
			macroblockEscape = nextBits(11);
			counter ++;
		}
		//printf("macroblock counter: %d\n", counter);
		
		int symbol;
		symbol = this->macroblockAddressIncrementTable.decode(*this);
		this->macroblockAddressIncrement = (unsigned int)symbol;
		this->macroblockAddressIncrement += counter * 33;
		//printf("macroblockAddressIncrement: %d\n", this->macroblockAddressIncrement);
		
		this->macroblockAddress = this->previousMacroblockAddress;
		for(int i = 0; i < this->macroblockAddressIncrement - 1; i ++){
			this->macroblockAddress ++;
			this->mbRow = this->macroblockAddress / this->mbWidth;
			this->mbColumn = this->macroblockAddress % this->mbWidth;
			this->dctDcYPast = 1024;
			this->dctDcCbPast = 1024;
			this->dctDcCrPast = 1024;
			if(this->pictureCodingType == P_FRAME){
				fillSkippedMacroblock();
				
				this->reconRightFor = 0;
				this->reconDownFor = 0;
				this->reconRightForPrev = 0;
				this->reconDownForPrev = 0;
				this->rightFor = 0;
				this->downFor = 0;
				
				this->reconRightBack = 0;
				this->reconDownBack = 0;
				this->reconRightBackPrev = 0;
				this->reconDownBackPrev = 0;
				this->rightBack = 0;
				this->downBack = 0;
			}
			else if(this->pictureCodingType == B_FRAME){
				fillSkippedMacroblock();
				
				this->reconRightFor = this->reconRightForPrev;
				this->reconDownFor = this->reconDownForPrev;
				this->reconRightBack = this->reconRightBackPrev;
				this->reconDownBack = this->reconDownBackPrev;
			}
		}
		this->macroblockAddress ++;
		//this->macroblockAddress = this->previousMacroblockAddress + this->macroblockAddressIncrement;
		//printf("macroblockAddress: %d\n", this->macroblockAddress);
		this->previousMacroblockAddress = this->macroblockAddress;
		
		this->mbRow = this->macroblockAddress / this->mbWidth;
		this->mbColumn = this->macroblockAddress % this->mbWidth;
		//printf("mbRow: %d, mbColumn: %d\n", this->mbRow, this->mbColumn);
		
		switch(this->pictureCodingType){
			case I_FRAME:
				symbol = this->macroblockTypeITable.decode(*this);
				break;
			case P_FRAME:
				symbol = this->macroblockTypePTable.decode(*this);
				break;
			case B_FRAME:
				symbol = this->macroblockTypeBTable.decode(*this);
				break;
			case D_FRAME:
				symbol = nextBits(1);
				break;
		}
		fillMacroblockType(symbol);
		//printf("macroblockQuant: %x\n", this->macroblockQuant);
		//printf("macroblockMotionForward: %x\n", this->macroblockMotionForward);
		//printf("macroblockMotionBackward: %x\n", this->macroblockMotionBackward);
		//printf("macroblockPattern: %x\n", this->macroblockPattern);
		//printf("macroblockIntra: %x\n", this->macroblockIntra);
		
		if(this->macroblockQuant){
			this->quantizerScale = nextBits(5);
			//printf("quantizerScale: %x\n", this->quantizerScale);
		}
		
		if(this->macroblockMotionForward){
			this->motionHorizontalForwardCode = this->motionVectorTable.decode(*this);
			
			if(this->forwardF != 1 && this->motionHorizontalForwardCode != 0){
				this->motionHorizontalForwardR = nextBits(this->forwardRSize);
				//printf("motionHorizontalForwardR: %x\n", this->motionHorizontalForwardR);
			}
			
			this->motionVerticalForwardCode = this->motionVectorTable.decode(*this);
			if(this->forwardF != 1 && this->motionVerticalForwardCode != 0){
				this->motionVerticalForwardR = nextBits(this->forwardRSize);
				//printf("motionVerticalForwardR: %x\n", this->motionVerticalForwardR);
			}
		}
		else{
			if(this->pictureCodingType == P_FRAME){
				this->reconRightFor = 0;
				this->reconDownFor = 0;
				this->reconRightForPrev = 0;
				this->reconDownForPrev = 0;
				this->rightFor = 0;
				this->downFor = 0;
			}
			else if(this->pictureCodingType == B_FRAME){
				this->reconRightFor = this->reconRightForPrev;
				this->reconDownFor = this->reconDownForPrev;
			}
		}
		
		if(this->macroblockMotionBackward){
			this->motionHorizontalBackwardCode = this->motionVectorTable.decode(*this);
			
			if(this->backwardF != 1 && this->motionHorizontalBackwardCode != 0){
				this->motionHorizontalBackwardR = nextBits(this->backwardRSize);
				//printf("motionHorizontalBackwardR: %x\n", this->motionHorizontalBackwardR);
			}
			
			this->motionVerticalBackwardCode = this->motionVectorTable.decode(*this);
			if(this->backwardF != 1 && this->motionVerticalBackwardCode != 0){
				this->motionVerticalBackwardR = nextBits(this->backwardRSize);
				//printf("motionVerticalBackwardR: %x\n", this->motionVerticalBackwardR);
			}
		}
		else{
			if(this->pictureCodingType == B_FRAME){
				this->reconRightBack = this->reconRightBackPrev;
				this->reconDownBack = this->reconDownBackPrev;
			}
		}
		
		for(int i = 0; i < 6; i ++){
			this->patternCode[i] = 0;
			if(macroblockIntra){
				this->patternCode[i] = 1;
			}
		}
		if(!macroblockIntra){
			this->dctDcYPast = 1024;
			this->dctDcCbPast = 1024;
			this->dctDcCrPast = 1024;
		}
		
		if(this->macroblockPattern){
			this->cbp = this->macroblockPatternTable.decode(*this);
			//printf("codedBlockPattern: %d\n", this->cbp);
			for(int i = 0; i < 6; i ++){
				if(this->cbp & (1 << (5 - i))){
					this->patternCode[i] = 1;
				}
			}
		}
		
		if(this->pictureCodingType == P_FRAME && this->macroblockMotionForward){
			reconstructForwardMotionVector();
		}
		if(this->pictureCodingType == B_FRAME && this->macroblockMotionForward){
			reconstructForwardMotionVector();
		}
		if(this->pictureCodingType == B_FRAME && this->macroblockMotionBackward){
			reconstructBackwardMotionVector();
		}
		for(int i = 0; i < 6; i ++){
			block(i);
			switch(this->pictureCodingType){
				case I_FRAME:
					intraBlockDecoding(i);
					break;
				case P_FRAME:
					if(this->macroblockIntra){
						intraBlockDecoding(i);
					}
					else{
						nonIntraBlockPDecoding(i);
					}
					break;
				case B_FRAME:
					if(this->macroblockIntra){
						intraBlockDecoding(i);
						this->reconRightFor = 0;
						this->reconDownFor = 0;
						this->rightFor = 0;
						this->downFor = 0;
						this->reconRightBack = 0;
						this->reconDownBack = 0;
						this->rightBack = 0;
						this->downBack = 0;
					}
					else{
						nonIntraBlockBDecoding(i);
					}
			}

		}
		
		if(this->pictureCodingType == D_FRAME){
			unsigned int endOfMacroblock = nextBits(1);
			//printf("endOfMacroblock\n");
		}
	}
	
	void fillMacroblockType(unsigned int symbol){
		switch(this->pictureCodingType){
			case I_FRAME:
				switch(symbol){
					case 1:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 1;
						break;
					case 2:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 1;
						break;
				}
				break;
			case P_FRAME:
				switch(symbol){
					case 1:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 2:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 3:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 0;
						break;
					case 4:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 1;
						break;
					case 5:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 6:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 7:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 1;
						break;
				}
				break;
			case B_FRAME:
				switch(symbol){
					case 1:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 1;
						this->macroblockPattern = 0;
						this->macroblockIntra = 0;
						break;
					case 2:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 1;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 3:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 1;
						this->macroblockPattern = 0;
						this->macroblockIntra = 0;
						break;
					case 4:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 1;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 5:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 0;
						break;
					case 6:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 7:
						this->macroblockQuant = 0;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 1;
						break;
					case 8:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 1;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 9:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 1;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 10:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 1;
						this->macroblockPattern = 1;
						this->macroblockIntra = 0;
						break;
					case 11:
						this->macroblockQuant = 1;
						this->macroblockMotionForward = 0;
						this->macroblockMotionBackward = 0;
						this->macroblockPattern = 0;
						this->macroblockIntra = 1;
						break;
				}
				break;
			case D_FRAME:
				this->macroblockQuant = 0;
				this->macroblockMotionForward = 0;
				this->macroblockMotionBackward = 0;
				this->macroblockPattern = 0;
				this->macroblockIntra = 1;
				break;
		}
	}
	
	unsigned int dctDcSizeLuminance;
	unsigned int dctDcSizeChrominance;
	unsigned int dctDcDifferentialLuminance;
	unsigned int dctDcDifferentialChrominance;
	int dctZZ[64];
	void block(int index){

		blockArrayInitialize(this->dctZZ);
		int k = 0, run, level;
		//printf("\nblock %d\n", index);
		if(this->patternCode[index]){
			if(this->macroblockIntra){
				if(index < 4){
					this->dctDcSizeLuminance = (unsigned int)this->dctDcSizeLuminanceTable.decode(*this);
					//printf("dctDcSizeLuminance: %d\n", this->dctDcSizeLuminance);
					if(this->dctDcSizeLuminance != 0){
						this->dctDcDifferentialLuminance = (unsigned int)nextBits(this->dctDcSizeLuminance);
						//printf("dctDcDifferentialLuminance: %d\n", this->dctDcDifferentialLuminance);
						if(this->dctDcDifferentialLuminance & (1 << (this->dctDcSizeLuminance - 1))){
							dctZZ[0] = this->dctDcDifferentialLuminance;
						}
						else{
							dctZZ[0] = (-1 << this->dctDcSizeLuminance) | (this->dctDcDifferentialLuminance + 1);
						}
					}
				}
				else{
					this->dctDcSizeChrominance = (unsigned int)this->dctDcSizeChrominanceTable.decode(*this);
					//printf("dctDcSizeChrominance: %d\n", this->dctDcSizeChrominance);
					if(this->dctDcSizeChrominance != 0){
						this->dctDcDifferentialChrominance = (unsigned int)nextBits(this->dctDcSizeChrominance);
						//printf("dctDcDifferentialChrominance: %d\n", this->dctDcDifferentialChrominance);
						if(this->dctDcDifferentialChrominance & (1 << (this->dctDcSizeChrominance - 1))){
							dctZZ[0] = this->dctDcDifferentialChrominance;
						}
						else{
							dctZZ[0] = (-1 << this->dctDcSizeChrominance) | (this->dctDcDifferentialChrominance + 1);
						}
					}
				}
			}
			else{
				readRunLevel(run, level, true);
				k = run;
				dctZZ[k] = level;
				//printf("dctCoeffFirst run: %d, level: %d\n", run, level);
			}

			if(this->pictureCodingType != 4){
				while(nextBits(2, true) != 0x2){
		clock_t t1 = clock();
					readRunLevel(run, level);
					k += (run + 1);
					dctZZ[k] = level;
					//printf("dctCoeffNext run: %d, level: %d\n", run, level);
		clock_t t2 = clock();
		static int kk = 0;
		fprintf(stderr, "%d\n", t2 - t1);
		if(kk++ == 2) exit(0);
				}
				unsigned int endOfBlock = nextBits(2);
				//printf("EOB\n");
			}
		}

	}
	
	void blockArrayInitialize(int array[]){
		for(int i = 0; i < 64; i ++){
			array[i] = 0;
		}
	}
	
	void readRunLevel(int &run, int &level, bool first = false){
		int tempBitPtr = this->fileStartBit;
		run = (first)? this->dctCoeffFirstRunTable.decode(*this): this->dctCoeffNextRunTable.decode(*this);
		this->fileStartBit = tempBitPtr;
		level = (first)? this->dctCoeffFirstLevelTable.decode(*this): this->dctCoeffNextLevelTable.decode(*this);
		if(run == -1){ // run == escape
			run = nextBits(6);
			level = (int)nextBits(8);
			if(level >= 128){
				level -= 256;
			}
			switch(level){
				case -128:
					level = (int)nextBits(8) - 256;
					break;
				case 0:
					level = nextBits(8);
					break;
			}
		}
	}
	
	int sign(int value){
		if(value > 0){
			return 1;
		}
		if(value < 0){
			return -1;
		}
		return 0;
	}
	
	void reconstructForwardMotionVector(){
		if(this->forwardF == 1 || this->motionHorizontalForwardCode == 0){
			this->complementHorizontalForwardR = 0;
		}
		else{
			this->complementHorizontalForwardR = this->forwardF - 1 - this->motionHorizontalForwardR;
		}
		
		if(this->forwardF == 1 || this->motionVerticalForwardCode == 0){
			this->complementVerticalForwardR = 0;
		}
		else{
			this->complementVerticalForwardR = this->forwardF - 1 - this->motionVerticalForwardR;
		}
		
		this->rightLittle = this->motionHorizontalForwardCode * this->forwardF;
		if(this->rightLittle == 0){
			this->rightBig = 0;
		}
		else{
			if(this->rightLittle > 0){
				this->rightLittle -= this->complementHorizontalForwardR;
				this->rightBig = this->rightLittle - (this->forwardF << 5);
			}
			else{
				this->rightLittle += this->complementHorizontalForwardR;
				this->rightBig = this->rightLittle + (this->forwardF << 5);
			}
		}
		
		this->downLittle = this->motionVerticalForwardCode * this->forwardF;
		if(this->downLittle == 0){
			this->downBig = 0;
		}
		else{
			if(this->downLittle > 0){
				this->downLittle -= this->complementVerticalForwardR;
				this->downBig = this->downLittle - (this->forwardF << 5);
			}
			else{
				this->downLittle += this->complementVerticalForwardR;
				this->downBig = this->downLittle + (this->forwardF << 5);
			}
		}
		
		int max = (this->forwardF << 4) - 1;
		int min = -(forwardF << 4);
		
		int newVector = this->reconRightForPrev + this->rightLittle;
		if(newVector <= max && newVector >= min){
			this->reconRightFor = this->reconRightForPrev + this->rightLittle;
		}
		else{
			this->reconRightFor = this->reconRightForPrev + this->rightBig;
		}
		this->reconRightForPrev = this->reconRightFor;
		if(this->fullPelForwardVector){
			this->reconRightFor <<= 1;
		}
		
		newVector = this->reconDownForPrev + this->downLittle;
		if(newVector <= max && newVector >= min){
			this->reconDownFor = this->reconDownForPrev + this->downLittle;
		}
		else{
			this->reconDownFor = this->reconDownForPrev + this->downBig;
		}
		this->reconDownForPrev = this->reconDownFor;
		if(this->fullPelForwardVector){
			this->reconDownFor <<= 1;
		}
		
	}
	
	void reconstructBackwardMotionVector(){
		if(this->backwardF == 1 || this->motionHorizontalBackwardCode == 0){
			this->complementHorizontalBackwardR = 0;
		}
		else{
			this->complementHorizontalBackwardR = this->backwardF - 1 - this->motionHorizontalBackwardR;
		}
		
		if(this->backwardF == 1 || this->motionVerticalBackwardCode == 0){
			this->complementVerticalBackwardR = 0;
		}
		else{
			this->complementVerticalBackwardR = this->backwardF - 1 - this->motionVerticalBackwardR;
		}
		
		this->rightLittle = this->motionHorizontalBackwardCode * this->backwardF;
		if(this->rightLittle == 0){
			this->rightBig = 0;
		}
		else{
			if(this->rightLittle > 0){
				this->rightLittle -= this->complementHorizontalBackwardR;
				this->rightBig = this->rightLittle - (this->backwardF << 5);
			}
			else{
				this->rightLittle += this->complementHorizontalBackwardR;
				this->rightBig = this->rightLittle + (this->backwardF << 5);
			}
		}
		
		this->downLittle = this->motionVerticalBackwardCode * this->backwardF;
		if(this->downLittle == 0){
			this->downBig = 0;
		}
		else{
			if(this->downLittle > 0){
				this->downLittle -= this->complementVerticalBackwardR;
				this->downBig = this->downLittle - (this->backwardF << 5);
			}
			else{
				this->downLittle += this->complementVerticalBackwardR;
				this->downBig = this->downLittle + (this->backwardF << 5);
			}
		}
		
		int max = (this->backwardF << 4) - 1;
		int min = -(backwardF << 4);
		
		int newVector = this->reconRightBackPrev + this->rightLittle;
		if(newVector <= max && newVector >= min){
			this->reconRightBack = this->reconRightBackPrev + this->rightLittle;
		}
		else{
			this->reconRightBack = this->reconRightBackPrev + this->rightBig;
		}
		this->reconRightBackPrev = this->reconRightBack;
		if(this->fullPelBackwardVector){
			this->reconRightBack <<= 1;
		}
		
		newVector = this->reconDownBackPrev + this->downLittle;
		if(newVector <= max && newVector >= min){
			this->reconDownBack = this->reconDownBackPrev + this->downLittle;
		}
		else{
			this->reconDownBack = this->reconDownBackPrev + this->downBig;
		}
		this->reconDownBackPrev = this->reconDownBack;
		if(this->fullPelBackwardVector){
			this->reconDownBack <<= 1;
		}
	}
	
	void intraBlockDecoding(int index){
		int k = 0;
		int signedQuantizerScale = (int)this->quantizerScale;
		Block dctRecon;
		switch(index){
			case 0: // First Y
				for(int m = 0; m < 8; m ++){
					for(int n = 0; n < 8; n ++){
						k = this->scan.read(m, n);
						dctRecon.write(m, n, (2 * this->dctZZ[k] * signedQuantizerScale * this->intraQuantizerTable.read(m, n)) / 16);
						if(dctRecon.read(m, n) & 1 == 0){
							dctRecon.write(m, n, dctRecon.read(m, n) - sign(dctRecon.read(m, n))); 
						}
						if(dctRecon.read(m, n) > 2047){
							dctRecon.write(m, n, 2047);
						}
						if(dctRecon.read(m, n) < -2048){
							dctRecon.write(m, n, -2048); 
						}
					}
				}

				dctRecon.write(0, 0, this->dctZZ[0] * 8);
				if(this->macroblockAddress - this->pastIntraAddress > 1){
					//printf("this->macroblockAddress - this->pastIntraAddress > 1\n");
					dctRecon.write(0, 0, 1024 + dctRecon.read(0, 0));
				}
				else{
					dctRecon.write(0, 0, this->dctDcYPast + dctRecon.read(0, 0));
				}
				this->dctDcYPast = dctRecon.read(0, 0);

				break;
			case 1: // Other Y
			case 2:
			case 3:
				for(int m = 0; m < 8; m ++){
					for(int n = 0; n < 8; n ++){
						k = this->scan.read(m, n);
						dctRecon.write(m, n, (this->dctZZ[k] * signedQuantizerScale * this->intraQuantizerTable.read(m, n)) >> 3);
						if(dctRecon.read(m, n) & 1 == 0){
							dctRecon.write(m, n, dctRecon.read(m, n) - sign(dctRecon.read(m, n))); 
						}
						if(dctRecon.read(m, n) > 2047){
							dctRecon.write(m, n, 2047);
						}
						if(dctRecon.read(m, n) < -2048){
							dctRecon.write(m, n, -2048); 
						}
					}
				}


				dctRecon.write(0, 0, this->dctDcYPast + (this->dctZZ[0] << 3));
				this->dctDcYPast = dctRecon.read(0, 0);
				break;
			case 4: // Cb
				for(int m = 0; m < 8; m ++){
					for(int n = 0; n < 8; n ++){
						k = scan.read(m, n);
						dctRecon.write(m, n, (this->dctZZ[k] * signedQuantizerScale * this->intraQuantizerTable.read(m, n)) >> 3);
						if(dctRecon.read(m, n) & 1 == 0){
							dctRecon.write(m, n, dctRecon.read(m, n) - sign(dctRecon.read(m, n)));
						}
						if(dctRecon.read(m, n) > 2047){
							dctRecon.write(m, n, 2047);
						}
						if(dctRecon.read(m, n) < -2048){
							dctRecon.write(m, n, -2048); 
						}
					}
				}
				dctRecon.write(0, 0, dctZZ[0] << 3);
				if(this->macroblockAddress - this->pastIntraAddress > 1){
					dctRecon.write(0, 0, 128 * 8 + dctRecon.read(0, 0));
				}
				else{
					dctRecon.write(0, 0, this->dctDcCbPast + dctRecon.read(0, 0));
				}
				this->dctDcCbPast = dctRecon.read(0, 0);
				break;
			case 5: // Cr
				for(int m = 0; m < 8; m ++){
					for(int n = 0; n < 8; n ++){
						k = scan.read(m, n);
						dctRecon.write(m, n, (this->dctZZ[k] * signedQuantizerScale * this->intraQuantizerTable.read(m, n)) >> 3);
						if(dctRecon.read(m, n) & 1 == 0){
							dctRecon.write(m, n, dctRecon.read(m, n) - sign(dctRecon.read(m, n)));
						}
						if(dctRecon.read(m, n) > 2047){
							dctRecon.write(m, n, 2047);
						}
						if(dctRecon.read(m, n) < -2048){
							dctRecon.write(m, n, -2048); 
						}
					}
				}
				dctRecon.write(0, 0, dctZZ[0] << 3);
				if(this->macroblockAddress - this->pastIntraAddress > 1){
					dctRecon.write(0, 0, 1024 + dctRecon.read(0, 0));
				}
				else{
					dctRecon.write(0, 0, this->dctDcCrPast + dctRecon.read(0, 0));
				}
				this->dctDcCrPast = dctRecon.read(0, 0);
				break;
		}
		dctRecon.IDCT();

		dctRecon.positiveCells();

		putImagePixels(index, dctRecon);
	}
	
	void nonIntraBlockPDecoding(int index){
		
		if(index < 4){
			this->rightFor = this->reconRightFor >> 1;
			this->downFor = this->reconDownFor >> 1;
			this->rightHalfFor = this->reconRightFor - 2 * this->rightFor;
			this->downHalfFor = this->reconDownFor - 2 * this->downFor;
		}
		else{
			this->rightFor = (this->reconRightFor / 2) >> 1;
			this->downFor = (this->reconDownFor / 2) >> 1;
			this->rightHalfFor = this->reconRightFor / 2 - 2 * this->rightFor;
			this->downHalfFor = this->reconDownFor / 2 - 2 * this->downFor;
		}
		
		unsigned int horizontalPixelIndex;
		unsigned int verticalPixelIndex;
		Block pel;
		for(int i = 0; i < 8; i ++){
			for(int j = 0; j < 8; j ++){
				if(index < 4){
					horizontalPixelIndex = this->mbColumn * 16 + (index % 2) * 8 + j;
					verticalPixelIndex = this->mbRow * 16 + (index / 2) * 8 + i;
				}
				else{
					horizontalPixelIndex = this->mbColumn * 16 + j;
					verticalPixelIndex = this->mbRow * 16 + i;
				}
				
				if(horizontalPixelIndex >= this->horizontalSize || verticalPixelIndex >= this->verticalSize){
					continue;
				}
				
				switch(index){
					case 0:
					case 1:
					case 2:
					case 3:
						if(!this->rightHalfFor && !this->downHalfFor){
							Pixel ref1;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Y = 0;
							}
							pel.write(i, j, ref1.Y);
						}
						if(!this->rightHalfFor && this->downHalfFor){
							Pixel ref1, ref2;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Y = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
								ref2 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
							}
							else{
								ref2.Y = 0;
							}
							pel.write(i, j, (ref1.Y + ref2.Y + 1) / 2);
						}
						if(this->rightHalfFor && !this->downHalfFor){
							Pixel ref1, ref2;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Y = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
								ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref2.Y = 0;
							}
							pel.write(i, j, (ref1.Y + ref2.Y + 1) / 2);
						}
						if(this->rightHalfFor && this->downHalfFor){
							Pixel ref1, ref2, ref3, ref4;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Y = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
								ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref2.Y = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
								ref3 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
							}
							else{
								ref3.Y = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor + 1)){
								ref4 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref4.Y = 0;
							}
							pel.write(i, j, (ref1.Y + ref2.Y + ref3.Y + ref4.Y + 2) / 4);
						}
						break;
					case 4:
						if(!this->rightHalfFor && !this->downHalfFor){
							Pixel ref1;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cb = 0;
							}
							pel.write(i, j, ref1.Cb);
						}
						if(!this->rightHalfFor && this->downHalfFor){
							Pixel ref1, ref2;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cb = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
								ref2 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
							}
							else{
								ref2.Cb = 0;
							}
							pel.write(i, j, (ref1.Cb + ref2.Cb + 1) / 2);
						}
						if(this->rightHalfFor && !this->downHalfFor){
							Pixel ref1, ref2;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cb = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
								ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref2.Cb = 0;
							}
							pel.write(i, j, (ref1.Cb + ref2.Cb + 1) / 2);
						}
						if(this->rightHalfFor && this->downHalfFor){
							Pixel ref1, ref2, ref3, ref4;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cb = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
								ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref2.Cb = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
								ref3 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
							}
							else{
								ref3.Cb = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor + 1)){
								ref4 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref4.Cb = 0;
							}
							pel.write(i, j, (ref1.Cb + ref2.Cb + ref3.Cb + ref4.Cb + 2) / 4);
						}
						break;
					case 5:
						if(!this->rightHalfFor && !this->downHalfFor){
							Pixel ref1;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cr = 0;
							}
							pel.write(i, j, ref1.Cr);
						}
						if(!this->rightHalfFor && this->downHalfFor){
							Pixel ref1, ref2;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cr = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
								ref2 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
							}
							else{
								ref2.Cr = 0;
							}
							pel.write(i, j, (ref1.Cr + ref2.Cr + 1) / 2);
						}
						if(this->rightHalfFor && !this->downHalfFor){
							Pixel ref1, ref2;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cr = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
								ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref2.Cr = 0;
							}
							pel.write(i, j, (ref1.Cr + ref2.Cr + 1) / 2);
						}
						if(this->rightHalfFor && this->downHalfFor){
							Pixel ref1, ref2, ref3, ref4;
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
								ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
							}
							else{
								ref1.Cr = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
								ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref2.Cr = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
								ref3 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
							}
							else{
								ref3.Cr = 0;
							}
							if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor + 1)){
								ref4 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor + 1];
							}
							else{
								ref4.Cr = 0;
							}
							pel.write(i, j, (ref1.Cr + ref2.Cr + ref3.Cr + ref4.Cr + 2) / 4);
						}
						break;
				}
				
			}
		}
		
		Block dctRecon;
		int signedQuantizerScale = (int)this->quantizerScale;
		int k;
		if(this->patternCode[index]){
			for(int m = 0; m < 8 ; m ++){
				for(int n = 0; n < 8; n ++){
					k = this->scan.read(m, n);
					dctRecon.write(m, n, ((2 * this->dctZZ[k] + sign(this->dctZZ[k])) * signedQuantizerScale * this->nonIntraQuantizerTable.read(m, n)) / 16);
					if(dctRecon.read(m, n) & 1 == 0){
						dctRecon.write(m, n, dctRecon.read(m, n) - sign(dctRecon.read(m, n)));
					}
					if(dctRecon.read(m, n) > 2047){
						dctRecon.write(m, n, 2047);
					}
					if(dctRecon.read(m, n) < -2048){
						dctRecon.write(m, n, -2048);
					}
					if(this->dctZZ[k] == 0){
						dctRecon.write(m, n, 0);
					}
				}
			}
		}
		else{
			blockArrayInitialize(this->dctZZ);
			dctRecon.zigzag(this->dctZZ);
		}
		dctRecon.IDCT();
		
		Block completeDctRecon;
		for(int m = 0; m < 8; m ++){
			for(int n = 0; n < 8; n ++){
				completeDctRecon.write(m, n, dctRecon.read(m, n) + pel.read(m, n));
			}
		}
		completeDctRecon.positiveCells();
		putImagePixels(index, completeDctRecon);
		
	}
	
	void nonIntraBlockBDecoding(int index){
		Block pelFor;
		if(this->macroblockMotionForward){
			if(index < 4){
				this->rightFor = this->reconRightFor >> 1;
				this->downFor = this->reconDownFor >> 1;
				this->rightHalfFor = this->reconRightFor - 2 * this->rightFor;
				this->downHalfFor = this->reconDownFor - 2 * this->downFor;
			}
			else{
				this->rightFor = (this->reconRightFor / 2) >> 1;
				this->downFor = (this->reconDownFor / 2) >> 1;
				this->rightHalfFor = this->reconRightFor / 2 - 2 * this->rightFor;
				this->downHalfFor = this->reconDownFor / 2 - 2 * this->downFor;
			}
			
			unsigned int horizontalPixelIndex;
			unsigned int verticalPixelIndex;
			for(int i = 0; i < 8; i ++){
				for(int j = 0; j < 8; j ++){
					if(index < 4){
						horizontalPixelIndex = this->mbColumn * 16 + (index % 2) * 8 + j;
						verticalPixelIndex = this->mbRow * 16 + (index / 2) * 8 + i;
					}
					else{
						horizontalPixelIndex = this->mbColumn * 16 + j;
						verticalPixelIndex = this->mbRow * 16 + i;
					}
					
					if(horizontalPixelIndex >= this->horizontalSize || verticalPixelIndex >= this->verticalSize){
						continue;
					}
					
					switch(index){
						case 0:
						case 1:
						case 2:
						case 3:
							if(!this->rightHalfFor && !this->downHalfFor){
								Pixel ref1;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Y = 0;
								}
								pelFor.write(i, j, ref1.Y);
							}
							if(!this->rightHalfFor && this->downHalfFor){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
									ref2 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
								}
								else{
									ref2.Y = 0;
								}
								pelFor.write(i, j, (ref1.Y + ref2.Y + 1) / 2);
							}
							if(this->rightHalfFor && !this->downHalfFor){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
									ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref2.Y = 0;
								}
								pelFor.write(i, j, (ref1.Y + ref2.Y + 1) / 2);
							}
							if(this->rightHalfFor && this->downHalfFor){
								Pixel ref1, ref2, ref3, ref4;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
									ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref2.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
									ref3 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
								}
								else{
									ref3.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor + 1)){
									ref4 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref4.Y = 0;
								}
								pelFor.write(i, j, (ref1.Y + ref2.Y + ref3.Y + ref4.Y + 2) / 4);
							}
							break;
						case 4:
							if(!this->rightHalfFor && !this->downHalfFor){
								Pixel ref1;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cb = 0;
								}
								pelFor.write(i, j, ref1.Cb);
							}
							if(!this->rightHalfFor && this->downHalfFor){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
									ref2 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
								}
								else{
									ref2.Cb = 0;
								}
								pelFor.write(i, j, (ref1.Cb + ref2.Cb + 1) / 2);
							}
							if(this->rightHalfFor && !this->downHalfFor){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
									ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref2.Cb = 0;
								}
								pelFor.write(i, j, (ref1.Cb + ref2.Cb + 1) / 2);
							}
							if(this->rightHalfFor && this->downHalfFor){
								Pixel ref1, ref2, ref3, ref4;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
									ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref2.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
									ref3 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
								}
								else{
									ref3.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor + 1)){
									ref4 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref4.Cb = 0;
								}
								pelFor.write(i, j, (ref1.Cb + ref2.Cb + ref3.Cb + ref4.Cb + 2) / 4);
							}
							break;
						case 5:
							if(!this->rightHalfFor && !this->downHalfFor){
								Pixel ref1;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cr = 0;
								}
								pelFor.write(i, j, ref1.Cr);
							}
							if(!this->rightHalfFor && this->downHalfFor){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
									ref2 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
								}
								else{
									ref2.Cr = 0;
								}
								pelFor.write(i, j, (ref1.Cr + ref2.Cr + 1) / 2);
							}
							if(this->rightHalfFor && !this->downHalfFor){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
									ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref2.Cr = 0;
								}
								pelFor.write(i, j, (ref1.Cr + ref2.Cr + 1) / 2);
							}
							if(this->rightHalfFor && this->downHalfFor){
								Pixel ref1, ref2, ref3, ref4;
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor)){
									ref1 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor];
								}
								else{
									ref1.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor, horizontalPixelIndex + rightFor + 1)){
									ref2 = this->lastImage[verticalPixelIndex + downFor][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref2.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor)){
									ref3 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor];
								}
								else{
									ref3.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downFor + 1, horizontalPixelIndex + rightFor + 1)){
									ref4 = this->lastImage[verticalPixelIndex + downFor + 1][horizontalPixelIndex + rightFor + 1];
								}
								else{
									ref4.Cr = 0;
								}
								pelFor.write(i, j, (ref1.Cr + ref2.Cr + ref3.Cr + ref4.Cr + 2) / 4);
							}
							break;
					}
					
				}
			}
		}
		
		Block pelBack;
		if(this->macroblockMotionBackward){
			if(index < 4){
				this->rightBack = this->reconRightBack >> 1;
				this->downBack = this->reconDownBack >> 1;
				this->rightHalfBack = this->reconRightBack - 2 * this->rightBack;
				this->downHalfBack = this->reconDownBack - 2 * this->downBack;
			}
			else{
				this->rightBack = (this->reconRightBack / 2) >> 1;
				this->downBack = (this->reconDownBack / 2) >> 1;
				this->rightHalfBack = this->reconRightBack / 2 - 2 * this->rightBack;
				this->downHalfBack = this->reconDownBack / 2 - 2 * this->downBack;
			}
			
			unsigned int horizontalPixelIndex;
			unsigned int verticalPixelIndex;
			for(int i = 0; i < 8; i ++){
				for(int j = 0; j < 8; j ++){
					if(index < 4){
						horizontalPixelIndex = this->mbColumn * 16 + (index % 2) * 8 + j;
						verticalPixelIndex = this->mbRow * 16 + (index / 2) * 8 + i;
					}
					else{
						horizontalPixelIndex = this->mbColumn * 16 + j;
						verticalPixelIndex = this->mbRow * 16 + i;
					}
					
					if(horizontalPixelIndex >= this->horizontalSize || verticalPixelIndex >= this->verticalSize){
						continue;
					}
					
					switch(index){
						case 0:
						case 1:
						case 2:
						case 3:
							if(!this->rightHalfBack && !this->downHalfBack){
								Pixel ref1;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Y = 0;
								}
								pelBack.write(i, j, ref1.Y);
							}
							if(!this->rightHalfBack && this->downHalfBack){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack)){
									ref2 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack];
								}
								else{
									ref2.Y = 0;
								}
								pelBack.write(i, j, (ref1.Y + ref2.Y + 1) / 2);
							}
							if(this->rightHalfBack && !this->downHalfBack){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack + 1)){
									ref2 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref2.Y = 0;
								}
								pelBack.write(i, j, (ref1.Y + ref2.Y + 1) / 2);
							}
							if(this->rightHalfBack && this->downHalfBack){
								Pixel ref1, ref2, ref3, ref4;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack + 1)){
									ref2 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref2.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack)){
									ref3 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack];
								}
								else{
									ref3.Y = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack + 1)){
									ref4 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref4.Y = 0;
								}
								pelBack.write(i, j, (ref1.Y + ref2.Y + ref3.Y + ref4.Y + 2) / 4);
							}
							break;
						case 4:
							if(!this->rightHalfBack && !this->downHalfBack){
								Pixel ref1;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cb = 0;
								}
								pelBack.write(i, j, ref1.Cb);
							}
							if(!this->rightHalfBack && this->downHalfBack){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack)){
									ref2 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack];
								}
								else{
									ref2.Cb = 0;
								}
								pelBack.write(i, j, (ref1.Cb + ref2.Cb + 1) / 2);
							}
							if(this->rightHalfBack && !this->downHalfBack){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack + 1)){
									ref2 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref2.Cb = 0;
								}
								pelBack.write(i, j, (ref1.Cb + ref2.Cb + 1) / 2);
							}
							if(this->rightHalfBack && this->downHalfBack){
								Pixel ref1, ref2, ref3, ref4;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack + 1)){
									ref2 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref2.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack)){
									ref3 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack];
								}
								else{
									ref3.Cb = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack + 1)){
									ref4 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref4.Cb = 0;
								}
								pelBack.write(i, j, (ref1.Cb + ref2.Cb + ref3.Cb + ref4.Cb + 2) / 4);
							}
							break;
						case 5:
							if(!this->rightHalfBack && !this->downHalfBack){
								Pixel ref1;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cr = 0;
								}
								pelBack.write(i, j, ref1.Cr);
							}
							if(!this->rightHalfBack && this->downHalfBack){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack)){
									ref2 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack];
								}
								else{
									ref2.Cr = 0;
								}
								pelBack.write(i, j, (ref1.Cr + ref2.Cr + 1) / 2);
							}
							if(this->rightHalfBack && !this->downHalfBack){
								Pixel ref1, ref2;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack + 1)){
									ref2 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref2.Cr = 0;
								}
								pelBack.write(i, j, (ref1.Cr + ref2.Cr + 1) / 2);
							}
							if(this->rightHalfBack && this->downHalfBack){
								Pixel ref1, ref2, ref3, ref4;
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack)){
									ref1 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack];
								}
								else{
									ref1.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack, horizontalPixelIndex + rightBack + 1)){
									ref2 = this->nextImage[verticalPixelIndex + downBack][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref2.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack)){
									ref3 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack];
								}
								else{
									ref3.Cr = 0;
								}
								if(inImageBoundary(verticalPixelIndex + downBack + 1, horizontalPixelIndex + rightBack + 1)){
									ref4 = this->nextImage[verticalPixelIndex + downBack + 1][horizontalPixelIndex + rightBack + 1];
								}
								else{
									ref4.Cr = 0;
								}
								pelBack.write(i, j, (ref1.Cr + ref2.Cr + ref3.Cr + ref4.Cr + 2) / 4);
							}
							break;
					}
					
				}
			}
		}
		
		Block pel;
		for(int m = 0; m < 8; m ++){
			for(int n = 0; n < 8; n ++){
				if(this->macroblockMotionForward && this->macroblockMotionBackward){
					pel.write(m, n, (pelFor.read(m, n) + pelBack.read(m, n) + 1) / 2);
				}
				if(this->macroblockMotionForward && !this->macroblockMotionBackward){
					pel.write(m, n, pelFor.read(m, n));
				}
				if(!this->macroblockMotionForward && this->macroblockMotionBackward){
					pel.write(m, n, pelBack.read(m, n));
				}
			}
		}
		
		Block dctRecon;
		int signedQuantizerScale = (int)this->quantizerScale;
		int k;
		if(this->patternCode[index]){
			for(int m = 0; m < 8 ; m ++){
				for(int n = 0; n < 8; n ++){
					k = this->scan.read(m, n);
					dctRecon.write(m, n, ((2 * this->dctZZ[k] + sign(this->dctZZ[k])) * signedQuantizerScale * this->nonIntraQuantizerTable.read(m, n)) / 16);
					if(dctRecon.read(m, n) & 1 == 0){
						dctRecon.write(m, n, dctRecon.read(m, n) - sign(dctRecon.read(m, n)));
					}
					if(dctRecon.read(m, n) > 2047){
						dctRecon.write(m, n, 2047);
					}
					if(dctRecon.read(m, n) < -2048){
						dctRecon.write(m, n, -2048);
					}
					if(this->dctZZ[k] == 0){
						dctRecon.write(m, n, 0);
					}
				}
			}
		}
		else{
			blockArrayInitialize(this->dctZZ);
			dctRecon.zigzag(this->dctZZ);
		}
		dctRecon.IDCT();
		
		Block completeDctRecon;
		for(int m = 0; m < 8; m ++){
			for(int n = 0; n < 8; n ++){
				completeDctRecon.write(m, n, dctRecon.read(m, n) + pel.read(m, n));
			}
		}
		completeDctRecon.positiveCells();
		putImagePixels(index, completeDctRecon);
	}
	
	bool inImageBoundary(unsigned int y, unsigned int x){
		return x >= 0 && x < this->horizontalSize && y >= 0 && y < this->verticalSize;
	}
	
	void fillSkippedMacroblock(){
		unsigned int horizontalPixelIndex;
		unsigned int verticalPixelIndex;
		for(int m = 0; m < 16; m ++){
			for(int n = 0; n < 16; n ++){
				horizontalPixelIndex = (this->mbColumn << 4) + n;
				verticalPixelIndex = (this->mbRow << 4) + m;
				if(verticalPixelIndex >= this->verticalSize || horizontalPixelIndex >= this->horizontalSize){
					continue;
				}
				//this->image[verticalPixelIndex][horizontalPixelIndex] = this->lastImage[verticalPixelIndex][horizontalPixelIndex];
				this->image[verticalPixelIndex][horizontalPixelIndex] = this->nextImage[verticalPixelIndex][horizontalPixelIndex];
			}
		}
	}
	
	void putImagePixels(int index, Block &dctRecon){
		unsigned int horizontalPixelIndex;
		unsigned int verticalPixelIndex;
		
		switch(index){
			case 0:
			case 1:
			case 2:
			case 3:
				for(int m = 0; m < 8; m ++){
					for(int n = 0; n < 8; n ++){
						horizontalPixelIndex = (this->mbColumn << 4) + ((index & 1) << 3) + n;
						verticalPixelIndex = (this->mbRow << 4) + ((index >> 1) << 3) + m;
						if(horizontalPixelIndex >= this->horizontalSize || verticalPixelIndex >= this->verticalSize){
							continue;
						}

						this->image[verticalPixelIndex][horizontalPixelIndex].Y = dctRecon.read(m, n);
					}
				}
				break;
			case 4:
			case 5:
				for(int m = 0; m < 8; m ++){
					for(int n = 0; n < 8; n ++){
						horizontalPixelIndex = (this->mbColumn << 4) + (n << 1);
						verticalPixelIndex = (this->mbRow << 4) + (m << 1);
						if(index == 4){
							this->image[verticalPixelIndex][horizontalPixelIndex].Cb = dctRecon.read(m, n);
							this->image[verticalPixelIndex][horizontalPixelIndex + 1].Cb = dctRecon.read(m, n);
							this->image[verticalPixelIndex + 1][horizontalPixelIndex].Cb = dctRecon.read(m, n);
							this->image[verticalPixelIndex + 1][horizontalPixelIndex + 1].Cb = dctRecon.read(m, n);
						}
						else{
							this->image[verticalPixelIndex][horizontalPixelIndex].Cr = dctRecon.read(m, n);
							this->image[verticalPixelIndex][horizontalPixelIndex + 1].Cr = dctRecon.read(m, n);
							this->image[verticalPixelIndex + 1][horizontalPixelIndex].Cr = dctRecon.read(m, n);
							this->image[verticalPixelIndex + 1][horizontalPixelIndex + 1].Cr = dctRecon.read(m, n);

						}

					}
				}
				break;
		}


	}
};

int main(int argc, char *argv[]){
	MPEGDecoder mpeg;
	mpeg.decode(argv[1], argv[2]);
	return 0;
}
