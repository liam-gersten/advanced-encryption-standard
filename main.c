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
    int count;
};

typedef void free_matrix_fn(matrix *m);

void free_matrix(matrix *m) {
    free(m->vals);
    free(m);
}

void free_state(state *s, free_matrix_fn *F) {
    printf("freeing state\n");
    if (F != NULL) {
        for (int i = 0; i < s->count; i++) {
            (*F)((s->chunks)[i]);
        }
    }
    free(s->chunks);
    free(s);
}

int size_to_rounds(size_t key_size) {
    if (key_size == 128) return 10;
    else if (key_size == 192) return 12;
    return 14;
}

int coords_to_index(int row, int col) {
    return (row * 4) + col;
}

int index_to_row(int index) {
    return (index / 4);
}

int index_to_col(int index) {
    return (index % 4);
}

void print_matrix(matrix *A) {
    printf("\n");
    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            int index = coords_to_index(row, col);
            printf(" %c ", (A->vals)[index]);
        }
        printf("\n");
    }
    printf("\n");
}

void transpose(matrix *A) {
    char *tmp = calloc(sizeof(char), 16);
    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            int old_index = coords_to_index(row, col);
            int new_index = coords_to_index(col, row);
            tmp[new_index] = (A->vals)[old_index];
        }
    }
    free(A->vals);
    A->vals = tmp;
}

void add_scalar(matrix *A, int scalar) {
    for (size_t i = 0; i < 16; i++) {
        char new_value = (char)(((int)((A->vals)[i])) + scalar);
        (A->vals)[i] = new_value;
    }
}

void multiply_matrices(matrix *A, matrix *B, free_matrix_fn *F) {
    printf("multiplying matrices\n");
    char *tmp = calloc(sizeof(char), 16);
    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            int tmp_char = 0;
            for (size_t other = 0; other < 4; other++) {
                int index1 = coords_to_index(row, (int)other);
                int index2 = coords_to_index((int)other, col);
                int prod1 = (int)((A->vals)[index1]);
                int prod2 = (int)((B->vals)[index2]);
                tmp_char = tmp_char + (prod1 * prod2);
            }
            int tmp_index = coords_to_index(row, col);
            tmp[tmp_index] = (char)tmp_char;
        }
    }
    free(A->vals);
    A->vals = tmp;
    if (F != NULL) {
        (*F)(B);
    }
}

char *comprise_text(state *current_state) {
    printf("comprising text\n");
    int char_count = 0;
    char *text = calloc(sizeof(char), (current_state->count)*16);
    for (int i = 0; i < current_state->count; i++) {
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                int index = coords_to_index(row, col);
                char tmp = (((current_state->chunks)[i])->vals)[index];
                if (tmp == '\0') {
                    return text;
                } else {
                    text[char_count] = tmp;
                    char_count++;
                }
            }
        }
    }
    return text;
}

state *comprise_chunks(char *text, int text_length) {
    printf("comprising chunks\n");
    int char_count = 0;
    int chunk_count = text_length/16;
    if (text_length % 16 != 0) {
        chunk_count++;
    }
    state *current_state = malloc(sizeof(state));
    current_state->chunks = calloc(sizeof(matrix*), (size_t)chunk_count);
    for (int i = 0; i < chunk_count; i++) {
        matrix *current_chunk = malloc(sizeof(matrix));
        char *vals = calloc(sizeof(char), 16);
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                int index = coords_to_index(row, col);
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
    printf("\nKey = %s\n", key);
    fclose(f);
    return key;
}

matrix **expand_key(char *key, size_t key_size) {
    int rounds = size_to_rounds(key_size);
    matrix **keys = calloc((size_t)rounds, sizeof(matrix*));

    // key expansion //

    return keys;
}

void encrypt(char *text, char *tmp_key, size_t text_length, size_t key_size) {
    printf("encrypting\n");
    char *key;
    if (tmp_key == NULL) key = generate_key(key_size);
    else key = tmp_key;
    state *current_state = comprise_chunks(text, text_length);

    // encryption //

    char *new_text = comprise_text(current_state);
    printf("%s\n", new_text);
    free(key);
    free(new_text);
    free_state(current_state, &free_matrix);
}

void decrypt(char *text, char *key, size_t text_length, size_t key_size) {
    printf("decrypting\n");
    state *current_state = comprise_chunks(text, text_length);

    // decryption //

    char *new_text = comprise_text(current_state);
    printf("%s\n", new_text);
    free(key);
    free(new_text);
    free_state(current_state, &free_matrix);
}

int main(int argc, char *argv[]) {
    char *h = "hello hello hello hello hello hello hello hello hello";

    char *old_text = calloc(sizeof(char), strlen(h));
    char *text = strcpy(old_text, h);
    encrypt(text, NULL, strlen(h), 256);

    // char *old_text = calloc(sizeof(char), strlen(argv[1]));
    // char *text = strcpy(old_text, argv[1]);
    // encrypt(text, NULL, strlen(argv[1]), 256);

    free(text);
    // free(old_text);
    system("leaks executablename");
    return 0;
}

// Format:
// encrypt/decrypt
// key_size/key
// text
