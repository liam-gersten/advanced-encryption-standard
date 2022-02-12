#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct matrix_info matrix;
struct matrix_info {
    char *vals;
};

typedef struct state_info state;
struct state_info {
    matrix **chunks;
    matrix *permutation;
    int count;
};

typedef matrix *make_permutation_fn();
typedef void rotate_word_fn(char *w);
typedef void free_matrix_fn(matrix *m);

void free_matrix(matrix *m) {
    free(m->vals);
    free(m);
}

void free_state(state *s, free_matrix_fn *F) {
    printf("freeing state\n");
    if (F != NULL) {
        for (int i = 0; i < s->count; i++) (*F)((s->chunks)[i]);
    }
    free(s->chunks);
    free(s->permutation->vals);
    free(s->permutation);
    free(s);
}

int size_to_rounds(size_t key_size) {
    if (key_size == 128) return 10;
    else if (key_size == 192) return 12;
    return 14;
}

int coords_to_index(int row, int col, int rows) {
    return (row * rows) + col;
}

int index_to_row(int index, int rows) {
    return (index / rows);
}

int index_to_col(int index, int cols) {
    return (index % cols);
}

int hex_to_row(char h) {
    char new_val = (h >> 4) & 0x0F;
    return (int)new_val;
}

int hex_to_col(char h) {
    return (int)(h & 0x0F);
}

void print_text(char *text, size_t text_length) {
    for (size_t i = 0; i < text_length; i++) {
        char c = text[i];
        if (c == '\0' || c == '\n' || c == '\t') printf("@");
        else printf("%c", c);
    }
}

void print_matrix(matrix *A) {
    printf("\n");
    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            int index = coords_to_index(row, col, 4);
            printf(" %c ", (A->vals)[index]);
        }
        printf("\n");
    }
    printf("\n");
}

void transpose(matrix *A) {
    printf("\t\ttransposing matrix\n");
    char *tmp = calloc(sizeof(char), 16);
    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            int old_index = coords_to_index(row, col, 4);
            int new_index = coords_to_index(col, row, 4);
            tmp[new_index] = (A->vals)[old_index];
        }
    }
    free(A->vals);
    A->vals = tmp;
}

void add_scalar(matrix *A, int scalar) {
    printf("\t\tadding scalar\n");
    for (size_t i = 0; i < 16; i++) {
        char new_value = (char)(((int)((A->vals)[i])) + scalar);
        (A->vals)[i] = new_value;
    }
}

char gf_mult_recursive(char a, char b) {  // REQUIRES a >= 0 //
    if (a == 0x02) return b << 1;
    return b ^ gf_mult_recursive((char)(((int)a) - 1), b);
}

char gf_mult(char a, char b) {
    if ((int)a == 1) return b;
    if ((int)a == 0) return a;
    if ((int)b < 0) return gf_mult_recursive(a, b) ^ 0x1B;
    return gf_mult_recursive(a, b);
}

void multiply_matrices(matrix *A, matrix *B, free_matrix_fn *F) {
    printf("\t\tmultiplying matrices\n");
    char *tmp = calloc(sizeof(char), 16);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            char tmp_char = 0;
            for (int other = 0; other < 4; other++) {
                int index1 = coords_to_index(row, other, 4);
                int index2 = coords_to_index(other, col, 4);
                char prod1 = (A->vals)[index1];
                char prod2 = (B->vals)[index2];
                tmp_char = tmp_char ^ gf_mult(prod1, prod2);
            }
            tmp[coords_to_index(row, col, 4)] = tmp_char;
        }
    }
    free(B->vals);
    B->vals = tmp;
    if (F != NULL) (*F)(A);
}

matrix *comprise_matrix(char *key) {
    matrix *m = malloc(sizeof(matrix));
    char *values = calloc(sizeof(char), 16);
    for (size_t i = 0; i < 16; i++) values[i] = key[i];
    m->vals = values;
    return m;
}

matrix *mix_cols_matrix() {
    char key[] = {2, 3, 1, 1, 1, 2, 3, 1, 1, 1, 2, 3, 3, 1, 1, 2};
    return comprise_matrix(key);
}

matrix *inv_mix_cols_matrix() {
    char key[] = {0x0E, 0x0B, 0x0D, 0x09, 0x09, 0x0E, 0x0B, 0x0D, 0x0D, 0x09, 
                0x0E, 0x0B, 0x0B, 0x0D, 0x09, 0x0E};
    return comprise_matrix(key);
}

char *comprise_text(state *current_state) {
    printf("\ncomprising text from state\n\n");
    int char_count = 0;
    char *text = calloc(sizeof(char), (current_state->count)*16);
    for (int i = 0; i < current_state->count; i++) {
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                int index = coords_to_index(row, col, 4);
                char tmp = (((current_state->chunks)[i])->vals)[index];
                text[char_count] = tmp;
                char_count++;
            }
        }
    }
    return text;
}

state *comprise_state(char *text, int text_length, make_permutation_fn *F) {
    printf("\ncomprising state from text\n\n");
    int char_count = 0;
    int chunk_count = text_length/16;
    if (text_length % 16 != 0) {
        chunk_count++;
    }
    state *current_state = malloc(sizeof(state));
    current_state->permutation = (*F)();
    current_state->chunks = calloc(sizeof(matrix*), (size_t)chunk_count);
    for (int i = 0; i < chunk_count; i++) {
        matrix *current_chunk = malloc(sizeof(matrix));
        char *vals = calloc(sizeof(char), 16);
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                int index = coords_to_index(row, col, 4);
                if (char_count < text_length) {
                    vals[index] = text[char_count];
                } else {
                    vals[index] = '\0';
                }
                char_count++;
            }
        }
        current_chunk->vals = vals;
        print_matrix(current_chunk);
        (current_state->chunks)[i] = current_chunk;
    }
    current_state->count = chunk_count;
    return current_state;
}

char *generate_key(size_t key_size) {
    printf("generating key\n");
    size_t char_number = key_size/8;
    char *key = calloc(sizeof(char), char_number);
    FILE *f;
    f = fopen("/dev/random", "r");
    for (size_t i = 0; i < char_number; i++) {
        char tmp;
        fread(&tmp, sizeof(tmp), 1, f);
        key[i] = tmp;
    }
    printf("\nKey = %s\n\n", key);
    fclose(f);
    return key;
}

char get_round_constant(int round) {
    if (round == 1) return 1;
    return gf_mult(2, get_round_constant(round - 1));
}

void word_substitute(char *word, char *sbox) {
    printf("\t\tword substitution\n");
    for (size_t i = 0; i < 4; i++) {
        char byte = word[i];
        int row = hex_to_row(byte);
        int col = hex_to_col(byte);
        int index = coords_to_index(row, col, 16);
        word[i] = sbox[index];
    }
}

void left_rotate_word(char *word) {
    char byte_1 = word[0];
    for (size_t i = 1; i < 4; i++) word[i - 1] = word[i];
    word[3] = byte_1;
}

void right_rotate_word(char *word) {
    char byte_4 = word[3];
    for (int i = 2; i >= 0; i--) word[i + 1] = word[i];
    word[0] = byte_4;
}

int subexpansion(int word, int round, char *sbox) {
    printf("\tsubexpansion\n");
    int *tmp_word = malloc(sizeof(int));
    *tmp_word = word;
    char *new_word = (char*)tmp_word;
    left_rotate_word(new_word);
    word_substitute(new_word, sbox);
    int round_constant = ((int)get_round_constant(round)) << 24;
    int result = (*((int*)new_word)) ^ round_constant;
    free(new_word);
    return result;
}

char *construct_sbox() {
    printf("\tconstructing sbox\n");
    char box[] = {0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 
        0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 
        0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 
        0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 
        0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 
        0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 
        0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 
        0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 
        0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 
        0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 
        0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 
        0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 
        0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 
        0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 
        0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 
        0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 
        0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 
        0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 
        0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 
        0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 
        0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 
        0x0f, 0xb0, 0x54, 0xbb, 0x16};
    char *new_sbox = calloc(sizeof(char), 256);
    for (size_t i = 0; i < 256; i++) new_sbox[i] = box[i];
    return new_sbox;
}

char *construct_inverse_sbox() {
    printf("\tconstructing inverse sbox\n");
    char box[] = {0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 
        0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 
        0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 
        0xcb, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 
        0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, 0x08, 0x2e, 0xa1, 0x66, 0x28, 
        0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
        0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 
        0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 
        0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, 0x90, 
        0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 
        0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 
        0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 
        0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 
        0xb4, 0xe6, 0x73, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 
        0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 
        0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 
        0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 
        0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, 0x1f, 0xdd, 0xa8, 0x33, 
        0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 
        0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 
        0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 
        0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
        0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 
        0x63, 0x55, 0x21, 0x0c, 0x7d};
    char *new_sbox = calloc(sizeof(char), 256);
    for (size_t i = 0; i < 256; i++) new_sbox[i] = box[i];
    return new_sbox;
}

int *expand_key(char *key, size_t key_size, char *sbox) {
    printf("expanding key\n");
    size_t rounds = (size_t)size_to_rounds(key_size);
    int *keys = calloc(4 + rounds, sizeof(int));
    for (size_t i = 0; i < 4; i++) keys[i] = ((int*)key)[i];
    int tmp;
    for (size_t i = 4; i < 4 * (rounds + 1); i++) {
        tmp = keys[i - 1];
        if (i % 4 == 0) {  // i divisible by 4 special case
            tmp = subexpansion(tmp, i / 4, sbox);
        }
        keys[i] = tmp ^ keys[i - 4];
    }
    printf("\ncipher keys = ");
    char *c_k = (char*)keys;
    for (size_t i = 0; i < 16 * (1 + rounds); i++) {
        if (i % 16 == 0) printf("\n0x ");
        if (c_k[i] == ' ' || c_k[i] == '\n' || c_k[i] == '\t') {
            printf("@");
        } else {
            printf("%c", c_k[i]);
        }
    }
    printf("\n\n");
    return keys;
}

void copy_words(int *src, int *dest, size_t start) {
    for (size_t i = 0; i < 4; i++) dest[i] = src[i + start];
}

void substitute_bytes(matrix *text, char *sbox) {  // pass rotate function
    printf("\tbyte substitution");
    for (size_t i = 0; i < 16; i++) {
        int row = hex_to_row((text->vals)[i]);
        int col = hex_to_col((text->vals)[i]);
        char new_element = sbox[coords_to_index(row, col, 16)];
        (text->vals)[i] = new_element;
    }
}

void shift_rows(matrix *text, rotate_word_fn *F) {
    printf("\tshifting rows");
    for (size_t row_num = 1; row_num < 4; row_num++) {
        char *row = (text->vals)+(row_num * 4);
        for (size_t i = 0; i < row_num; i++) (*F)(row);
    }
}

void mix_columns(matrix *A, matrix *B) {
    printf("\tmixing columns");
    multiply_matrices(A, B, NULL);
}

void add_round_key(matrix *text, int *words) {
    printf("\tadding round key");
    return;
}

void cipher_round(matrix *text, char *sbox, int *words, matrix *perm) {
    substitute_bytes(text, sbox);
    shift_rows(text, &left_rotate_word);
    mix_columns(perm, text);
    add_round_key(text, words);
    printf("\n");
}

void inverse_cipher_round(matrix *text, char *sbox, int *words, matrix *perm) {
    shift_rows(text, &right_rotate_word);
    substitute_bytes(text, sbox);
    add_round_key(text, words);
    mix_columns(perm, text);
    printf("\n");
}

char *remove_padding(char *old_text, size_t *text_length) {
    size_t new_length = 0;
    for (size_t i = 0; i < *text_length; i++) {
        if (old_text[i] == '\0') break;
        new_length++;
    }
    char *new_text = calloc(sizeof(char), new_length);
    for (size_t i = 0; i < new_length; i++) new_text[i] = old_text[i];
    free(old_text);
    *text_length = new_length;
    return new_text;
}

void replace_text(char *old_text, state *current_state) {
    printf("replacing text\n");
    char *new_text = comprise_text(current_state);
    for (size_t i = 0; i < (current_state->count)*16; i++) {
        old_text[i] = new_text[i];
    }
    free(new_text);
    free_state(current_state, &free_matrix);
}

char *encrypt(char *old_text, size_t *text_length, size_t key_size) {
    printf("encrypting\n");
    char *key = generate_key(key_size);
    state *current_state = comprise_state(old_text, *text_length, 
                                          &mix_cols_matrix);
    *text_length = 16 * (size_t)(current_state->count);
    char *sbox = construct_sbox();
    int *cipher_keys = expand_key(key, key_size, sbox);
    size_t round_number = size_to_rounds(key_size);
    int *words = calloc(sizeof(int), 4);
    for (size_t i = 0; i < current_state->count; i++) {
        matrix *text = (current_state->chunks)[i]; 
        printf("\ncipher round number 0");
        copy_words(cipher_keys, words, 0);
        add_round_key(text, words);
        printf("\n");
        for (size_t c_round = 1; c_round < round_number; c_round++) {
            printf("cipher round number %d", (int)c_round);
            copy_words(cipher_keys, words, (c_round * 4));
            cipher_round(text, sbox, words, current_state->permutation);
        }
        printf("cipher round number %d", (int)round_number);
        copy_words(cipher_keys, words, (round_number * 4));
        substitute_bytes(text, sbox);
        shift_rows(text, &left_rotate_word);
        add_round_key(text, words);
        printf("\n\n");
    }
    replace_text(old_text, current_state);
    free(words);
    free(sbox);
    free(cipher_keys);
    return key;
}

void decrypt(char *old_text, char *key, size_t text_length, size_t key_size) {
    printf("decrypting\n");
    state *current_state = comprise_state(old_text, text_length, 
                                          &inv_mix_cols_matrix);
    char *sbox = construct_inverse_sbox();
    int *cipher_keys = expand_key(key, key_size, sbox);
    size_t round_number = size_to_rounds(key_size);
    int *words = calloc(sizeof(int), 4);
    for (size_t i = 0; i < current_state->count; i++) {
        matrix *text = (current_state->chunks)[i];
        printf("\ncipher round number 0");
        print_matrix(text);
        copy_words(cipher_keys, words, (round_number * 4));
        add_round_key(text, words);
        printf("\n");
        for (size_t c_round = 1; c_round < round_number; c_round++) {
            printf("cipher round number %d", (int)c_round);
            print_matrix(text);
            copy_words(cipher_keys, words, ((round_number - c_round) * 4));
            inverse_cipher_round(text, sbox, words, current_state->permutation);
        }
        printf("cipher round number %d", (int)round_number);
        print_matrix(text);
        copy_words(cipher_keys, words, 0);
        shift_rows(text, &right_rotate_word);
        substitute_bytes(text, sbox);
        add_round_key(text, words);
        print_matrix(text);
        printf("\n\n");
    }
    replace_text(old_text, current_state);
    free(words);
    free(sbox);
    free(cipher_keys);
}

int main(int argc, char *argv[]) {
    char *h = "This is a string of text to be used as input";
    // char *h = "abcdefghijklmnopabcdefghijklmnop";
    char *old_text = calloc(sizeof(char), strlen(h));
    char *text = strcpy(old_text, h);
    size_t key_size = 256;
    size_t *text_length = malloc(sizeof(size_t));
    *text_length = strlen(h);
    printf("\n\ntext length = %d\n\n\n", (int)(*text_length));
    printf("\noriginal text = ");
    print_text(text, *text_length);
    char *key = encrypt(text, text_length, key_size);
    printf("\n\ntext length = %d\n\n\n", (int)(*text_length));
    printf("\nencrypted text = ");
    print_text(text, *text_length);
    for (size_t i = 0; i < *text_length; i++) printf("0x%d ", (int)(text[i]));
    printf("\n\n");
    printf("\nencryption key = ");
    print_text(key, key_size/8);
    decrypt(text, key, *text_length, key_size);
    text = remove_padding(text, text_length);
    printf("\n\ntext length = %d\n\n\n", (int)(*text_length));
    // printf("\ndecrypted text = ");
    // print_text(text, *text_length);

    // char *old_text = calloc(sizeof(char), strlen(argv[1]));
    // char *text = strcpy(old_text, argv[1]);
    // encrypt(text, NULL, strlen(argv[1]), 256);

    free(text);
    free(key);
    free(text_length);
    system("leaks executablename");
    return 0;
}
