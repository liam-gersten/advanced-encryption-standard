#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct matrix chunk;
struct matrix {
    char **rows;
};

typedef struct state_info state;
struct state_info {
    chunk **chunks;
    int count;
};

char *comprise_text(state *current_state) {
    int char_count = (current_state->count)*16;
    int index = 0;
    char *text = calloc(sizeof(char), char_count);
    for (int i = 0; i < current_state->count; i++) {
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 3; col++) {
                text[index] = ((((current_state->chunks)[i])->rows)[row])[col];
                index++;
            }
        }
    }
    return text;
}

state *comprise_chunks(char *text, int text_length) {
    int char_count = 0;
    int chunk_count = text_length/16;
    if (text_length % 16 != 0) {
        chunk_count++;
    }
    state *current_state = malloc(sizeof(state));
    for (int i = 0; i < chunk_count; i++) {
        chunk *current_chunk = malloc(sizeof(chunk));
        for (int row = 0; row < 4; row++) {
            char *current_row = malloc(sizeof(char*));
            for (int col = 0; col < 4; col++) {
                if (char_count < text_length) {
                    current_row[col] = text[char_count];
                    printf("%c\n", text[char_count]);
                } else {
                    current_row[col] = '\0';
                }
                char_count++;
            }
            (current_chunk->rows)[row] = current_row;
        }
        (current_state->chunks)[i] = current_chunk;
    }
    current_state->count = chunk_count;
    return current_state;
}

void free_state(state *current_state) {
    for (int i = 0; i < current_state->count; i++) {
        for (int row = 0; row < 4; row++) {
            free(((current_state->chunks)[i]->rows)[row]);
        }
        free((current_state->chunks)[i]->rows);
        free((current_state->chunks)[i]);
    }
    free(current_state->chunks);
    free(current_state);
}

int main(int argc, char *argv[]) {
    printf("%s\n", argv[1]);
    char *old_text = calloc(sizeof(char), strlen(argv[1]));
    char *text = strcpy(old_text, argv[1]);
    state *current_state = comprise_chunks(text, strlen(argv[1]));
    char *new_text = comprise_text(current_state);
    printf("%s\n", new_text);
    free_state(current_state);
    free(text);
    return 0;
}