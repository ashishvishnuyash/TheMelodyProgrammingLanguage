#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readfile.h"


#define BUFFER_SIZE 1024

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s\n", filename);
        return NULL;
    }

    // Allocate initial memory for file content
    size_t capacity = BUFFER_SIZE;
    char* content = (char*)malloc(capacity);
    if (content == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t length = 0;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // Check if we need to expand the allocated memory
        if (length + bytes_read >= capacity) {
            capacity *= 2; // Double the capacity
            char* temp = realloc(content, capacity);
            if (temp == NULL) {
                fprintf(stderr, "Error: Memory reallocation failed\n");
                free(content);
                fclose(file);
                return NULL;
            }
            content = temp;
        }
        // Copy buffer to content
        memcpy(content + length, buffer, bytes_read);
        length += bytes_read;
    }

    // Trim excess memory
    char* temp = realloc(content, length + 1);
    if (temp != NULL) {
        content = temp;
    } else {
        fprintf(stderr, "Error: Memory reallocation failed\n");
        free(content);
        fclose(file);
        return NULL;
    }

    content[length] = '\0'; // Null-terminate the string
    fclose(file);
    return content;
}

void free_file_content(char* content) {
    free(content);
}
