#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PRINT_PROCESS 1 // 0: 암/복호화 과정 숨김, 1: 암/복호화 과정 출력

#define KEY_SIZE 10
#define BLOCK 8
#define DATA_TYPE int // short, int, long, long long

typedef unsigned char Byte;
typedef enum {false, true} bool;

DATA_TYPE strbin2keytype(char *strbin) {
    DATA_TYPE dec=0;
    int i;
    int str_size;

    str_size = strlen(strbin);

    for(i=0; i<str_size; i++) {
        if(strbin[i] == '1') {
            dec = dec | (1 << (str_size-1)-i);
        }
    }

    return dec;
}

void print_bin(char *str, DATA_TYPE data) {
    int i;

    printf("%s", str);
    for(i=sizeof(DATA_TYPE)*8-1; i>=0; i--) {
        if(data & ((int)pow(2, i)))
            printf("%d", 1);
        else
            printf("%d", 0);
        if(i%4==0)
            printf(" ");
    }
    printf("\n");
}

int circular_shift(int data, int max_bit, int index, int offset, int shift_value) {
    int range;
    int block;
    int shifted;

    range = (pow(2, offset+index)-1)-(pow(2, index)-1);
    block = data & range;
    data = data ^ block;

    shifted = range & ((block << shift_value) | (block >> (offset-shift_value)));

    return (data | shifted)&((int)(pow(2, max_bit)-1));
}

DATA_TYPE p10(DATA_TYPE key) {
    int pattern[KEY_SIZE] = {3, 5, 2, 7, 4, 10, 1, 9, 8, 6};
    DATA_TYPE after_p10 = 0;
    int i, value;

    for(i=0; i < KEY_SIZE; i++) {
        value = (key & (int)pow(2, KEY_SIZE - pattern[i]));
        if(value)
            after_p10 = after_p10 | (int)pow(2, KEY_SIZE-i-1);
    }

    return after_p10;
}

Byte p8(DATA_TYPE key) {
    int pattern[BLOCK] = {6, 3, 7, 4, 8, 5, 10, 9};
    Byte after_p8 = 0;
    int i, value;

    for(i=0; i < BLOCK; i++) {
        value = (key & (int)pow(2, KEY_SIZE - pattern[i]));
        if(value)
            after_p8 = after_p8 | (int)pow(2, BLOCK-i-1);
    }

    return after_p8;
}

void key_gen(DATA_TYPE key, Byte *key1, Byte *key2) {

    key = p10(key);

    key = circular_shift(key, 10, 0, 5, 1);
    key = circular_shift(key, 10, 5, 5, 1);

    *key1 = p8(key);

    key = circular_shift(key, 10, 0, 5, 2);
    key = circular_shift(key, 10, 5, 5, 2);

    *key2 = p8(key);
}

Byte ip(Byte plain_text) {
    int pattern[BLOCK] = {2, 6, 3, 1, 4, 8, 5, 7};
    Byte after_ip = 0;
    int i, value;

    for(i=0; i < BLOCK; i++) {
        value = (plain_text & (int)pow(2, BLOCK - pattern[i]));
        if(value)
            after_ip = after_ip | (int)pow(2, BLOCK-i-1);
    }

    return after_ip;
}

Byte ip_inverse(Byte data) {
    int pattern[BLOCK] = {4, 1, 3, 5, 7, 2, 8, 6};
    Byte after_ip = 0;
    int i, value;

    for(i=0; i < BLOCK; i++) {
        value = (data & (int)pow(2, BLOCK - pattern[i]));
        if(value)
            after_ip = after_ip | (int)pow(2, BLOCK-i-1);
    }

    return after_ip;
}

Byte ep(Byte right) {
    Byte after_ep = 0;
    int pattern[] = {4, 1, 2, 3, 2, 3, 4, 1};
    int i, value;

    for(i=0; i < BLOCK; i++) {
        value = (right & (int)pow(2, (BLOCK/2) - pattern[i]));
        if(value)
            after_ep = after_ep | (int)pow(2, BLOCK-i-1);
    }

    return after_ep;
}

Byte s0(Byte data) {
    int sbox[4][4] = {
        {1, 0, 3, 2},
        {3, 2, 1, 0},
        {0, 2, 1, 3},
        {3, 1, 3, 2}
    };
    int row, col;

    row = ((data & 0b10000000)/128)*2 + (data & 0b00010000)/16; // AD
    col = ((data & 0b01000000)/64)*2 + (data & 0b00100000)/32; // BC

    return sbox[row][col];
}

Byte s1(Byte data) {
    int sbox[4][4] = {
        {0, 1, 2, 3},
        {2, 0, 1, 3},
        {3, 0, 1, 0},
        {2, 1, 0, 3}
    };
    int row, col;

    row = ((data & 0b1000)/8*2) + (data & 0b0001)/1; // AD
    col = ((data & 0b0100)/4*2) + (data & 0b0010)/2; // BC

    return sbox[row][col];
}

Byte p4(Byte data) {
    int pattern[BLOCK/2] = {2, 4, 3, 1};
    Byte after_p4 = 0;
    int i, value;

    for(i=0; i < sizeof(pattern); i++) {
        value = (data & (int)pow(2, BLOCK/2 - pattern[i]));
        if(value)
            after_p4 = after_p4 | (int)pow(2, BLOCK/2-i-1);
    }

    return after_p4;
}

Byte f(Byte right, Byte key) {
    Byte result = 0;

    result = ep(right);
    // printf("after ep : "); print_bin(result);
    result = result ^ key;
    result = (s0(result) << 2) | s1(result);
    result = p4(result);

    return result;
}

Byte cryptogram(Byte plain_text, Byte key1, Byte key2) {
    Byte data;
    Byte left4bit, right4bit;

    // IP
    data = ip(plain_text);
#if PRINT_PROCESS
print_bin("after IP >> ", data);
#endif
    // Round 1
    right4bit = data & ((int)pow(2, BLOCK/2)-1);
    left4bit = data ^ right4bit;
    left4bit = left4bit ^ (f(right4bit, key1) << BLOCK/2);
#if PRINT_PROCESS
print_bin("after round1 >> ", left4bit | right4bit);
#endif
    // swap
    data = (left4bit >> (BLOCK/2)) | (right4bit << (BLOCK/2));
#if PRINT_PROCESS
print_bin("after swap >> ", data);
#endif
    // Round 2
    right4bit = data & ((int)pow(2, BLOCK/2)-1);
    left4bit = data ^ right4bit;
    left4bit = left4bit ^ (f(right4bit, key2) << BLOCK/2);
    data = left4bit | right4bit;
#if PRINT_PROCESS
print_bin("after round2 >> ", data);
#endif

    // IP-1
    data = ip_inverse(data);
#if PRINT_PROCESS
print_bin("after IP-1 >> ", data);
#endif
    return data;
}

Byte encrypt(Byte plain_text, DATA_TYPE key) {
    Byte key1, key2;

    key_gen(key, &key1, &key2);
#if PRINT_PROCESS
printf("[encrypt]\n");
#endif
    return cryptogram(plain_text, key1, key2);
}

Byte decrypt(Byte plain_text, DATA_TYPE key) {
    Byte key1, key2;

    key_gen(key, &key1, &key2);
#if PRINT_PROCESS
printf("[decrypt]\n");
#endif
    return cryptogram(plain_text, key2, key1);
}

int main(int argc, char *argv[]) {
    DATA_TYPE key = 151;// 679; // 1010100111
    Byte plain_text = 165;// 174; // 10101110
    Byte ciphertext = 0;
    Byte deciphertext = 0;

    if(argc > 1) {
        key = strbin2keytype(argv[1]);
        plain_text = strbin2keytype(argv[2]);
    }

    print_bin("key : ", key);
    print_bin("plain_text : ", plain_text);

    printf("\n");

    ciphertext = encrypt(plain_text, key);
    print_bin("ciphertext : ", ciphertext);

    printf("\n");

    deciphertext = decrypt(ciphertext, key);
    print_bin("deciphertext : ", deciphertext);

    return 0;
}
