#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t state [4][4];
uint8_t mulXORed(uint8_t a, uint8_t b);
void mixColumns(state* from);
void invMixColumns(state* a);
void addRoundKey(state* st, state* key);

state* readPlainTextBlock(FILE* fptr) {
state* arr=malloc(sizeof(state));
for(int i=0;i<4;i++) {
for(int k=0;k<4;k++) {
(*arr)[i][k] = fgetc(fptr);  //4x4 tekstiblokki luetaan state* arr structiin
}
}
return arr;
}

void leftShift(int row,state* arr) {
uint8_t tmp=0;
for(int x=0; x<row; x++) {

	tmp=(*arr)[row][0];
	(*arr)[row][0]=(*arr)[row][1];
	(*arr)[row][1]=(*arr)[row][2];
	(*arr)[row][2]=(*arr)[row][3];
	(*arr)[row][3]=tmp;

	}
}

void rightShift(int row,state* arr) {
uint8_t tmp=0;
for(int x=0; x<row; x++) {

	tmp=(*arr)[row][3];
	(*arr)[row][3]=(*arr)[row][2];
	(*arr)[row][2]=(*arr)[row][1];
	(*arr)[row][1]=(*arr)[row][0];
	(*arr)[row][0]=tmp;

	}
}
void shiftRow(state* arr) {
leftShift(1,arr);
leftShift(2,arr); 
leftShift(3,arr); //Shiftaillaan aina rivien määrä kertoja
}

void invShiftRow(state* arr) {
rightShift(1,arr);
rightShift(2,arr);
rightShift(3,arr);
}


typedef struct {
uint8_t i;
uint8_t j;
} intPair;

intPair constructIndices(uint8_t byte) {
intPair x= {0,0};
uint8_t mask=15; //MASK 00001111
x.j=byte&mask;   //byte&MASK == 0000XXXX
x.i=byte>>4;   //SHIFT RIGHT 4
return x;
}


typedef uint8_t Sbox [16][16];

Sbox* readSbox(FILE* fptr) {
uint8_t byte[256];
int i=0;
while(!feof(fptr)) {
fscanf(fptr,"%x",&byte[i]);
i++;
}
fclose(fptr);
Sbox* box = malloc(sizeof(Sbox)); //LUETAAN SBOXIIN ARVOT byte[]:stä
int x=0;
for(int i=0;i<16;i++) {
	for(int k=0;k<16;k++) {
		(*box)[i][k]=byte[x];
		x++;	
	}
}
return box;	
}

void subBytes(state* st, Sbox* box) {
for(int i=0;i<4;i++) {
	for(int k=0;k<4;k++) {
	intPair x = constructIndices((*st)[i][k]);
	(*st)[i][k]=(*box)[x.i][x.j];
	}
}
}

/**** MIX THE COLUMNS ****/
void mixColumns(state* from) { //"matrix" 1, 2, 3
    int x;
	state* tmp=malloc(sizeof(state));
    for(x = 0; x < 4; ++x) { //mulXORed(0x01,n) == n, joten sitä ei tarvitse tehdä erikseen
        (*tmp)[0][x] = mulXORed(0x02, (*from)[0][x]) ^ mulXORed(0x03, (*from)[1][x])
                ^ (*from)[2][x] ^ (*from)[3][x];
        (*tmp)[1][x] = (*from)[0][x] ^ mulXORed(0x02, (*from)[1][x])
                ^ mulXORed(0x03, (*from)[2][x]) ^ (*from)[3][x];
        (*tmp)[2][x] = (*from)[0][x] ^ (*from)[1][x] ^ mulXORed(0x02, (*from)[2][x])
                ^ mulXORed(0x03, (*from)[3][x]);
        (*tmp)[3][x] = mulXORed(0x03, (*from)[0][x]) ^ (*from)[1][x] ^ (*from)[2][x]
                ^ mulXORed(0x02, (*from)[3][x]);
    }
from=tmp;
}

/**** "DECRYPT" MIXCOLUMNS ****/ 
void invMixColumns(state* a) { //"matrix" 9, 11, 13, 14
    int x;
	state* to=malloc(sizeof(state));
    for(x = 0; x < 4; ++x) {
        (*to)[0][x] = mulXORed(0x0E, (*a)[0][x]) ^ mulXORed(0x0B, (*a)[1][x])
              ^ mulXORed(0x0D, (*a)[2][x]) ^ mulXORed(0x09, (*a)[3][x]);
        (*to)[1][x] = mulXORed(0x09, (*a)[0][x]) ^ mulXORed(0x0E, (*a)[1][x])
              ^ mulXORed(0x0B, (*a)[2][x]) ^ mulXORed(0x0D, (*a)[3][x]);
        (*to)[2][x] = mulXORed(0x0D, (*a)[0][x]) ^ mulXORed(0x09, (*a)[1][x])
              ^ mulXORed(0x0E, (*a)[2][x]) ^ mulXORed(0x0B, (*a)[3][x]);
        (*to)[3][x] = mulXORed(0x0B, (*a)[0][x]) ^ mulXORed(0x0D, (*a)[1][x])
              ^ mulXORed(0x09, (*a)[2][x]) ^ mulXORed(0x0E, (*a)[3][x]);
    }
a=to;
}

uint8_t mulXORed(uint8_t a, uint8_t b) {
    int i;
    uint8_t value=0;
     
    for(i = 0; i < 8; i++) {
        if((b & 1) == 1)
            value ^= a;
         
        if((a & 0x80) == 0x80) { //DEC 128 BIN 10000000 reached --> must divide
            a <<= 1;
            a  ^= 0x1b;    // DEC 27 BIN 00011011 or x^4+x^3+x+1
        } else {
            a <<= 1;
        }
         
        b >>= 1;
    }
     
    return value;
}

/**** Adds a key for the encryption round ****/
void addRoundKey(state* st, state* key) {
int x;
    for(x = 0; x < 4; ++x) { 
        (*st)[0][x] = (*st)[0][x] ^ (*key)[0][x];
        (*st)[1][x] = (*st)[1][x] ^ (*key)[1][x];
        (*st)[2][x] = (*st)[2][x] ^ (*key)[2][x];
        (*st)[3][x] = (*st)[3][x] ^ (*key)[3][x];
    }
}

int main(void) {
FILE * fpt;
FILE * fptr;
FILE * fptr1;
if ((fpt = fopen("teksti.txt", "r")) == NULL)
        {
            printf("Failed to open file: teksti.txt\n");
            return 0;
        } 
state* a = readPlainTextBlock(fpt);
fclose(fpt);  

if ((fptr = fopen("Sbox", "r")) == NULL)
        {
            printf("Failed to open file: Sbox\n");
            return 0;
        } 
Sbox* box=readSbox(fptr);
if ((fptr1 = fopen("invSbox","r")) == NULL) {
	printf("Failed to open file: invSbox\n");
	return 0;
}
Sbox* invSbox=readSbox(fptr1);


/**** roundkey ****/
state keya= {{41,123,32,15},{19,63,54,22},{55,196,222,17},{13,95,78,37}};
state* key=&keya;

printf("UNENCRYPTED:\n");
for(int i=0;i<4;i++) {
printf("%u\t%u\t%u\t%u\n",(*a)[i][0],(*a)[i][1],(*a)[i][2],(*a)[i][3]); }
printf("\n");

//ENCRYPT
subBytes(a,box);
shiftRow(a);
mixColumns(a);
addRoundKey(a,key);

printf("ENCRYPTED:\n");
for(int i=0;i<4;i++) {
printf("%u\t%u\t%u\t%u\n",(*a)[i][0],(*a)[i][1],(*a)[i][2],(*a)[i][3]); }
printf("\n");

//DECRYPT
addRoundKey(a,key);
invMixColumns(a);
invShiftRow(a);
subBytes(a,invSbox);

printf("DECRYPTED:\n");
for(int i=0;i<4;i++) {
printf("%u\t%u\t%u\t%u\n",(*a)[i][0],(*a)[i][1],(*a)[i][2],(*a)[i][3]); }
printf("\n");
printf("CHAR REPRESENTATION:\n");
for(int i=0;i<4;i++) {
printf("%c\t%c\t%c\t%c\n",(*a)[i][0],(*a)[i][1],(*a)[i][2],(*a)[i][3]); }
printf("\n");

}
