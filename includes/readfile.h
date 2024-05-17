#ifndef FILE_READER_H
#define FILE_READER_H

#ifdef __cplusplus
extern "C" {
#endif

// Function to read file contents into dynamically allocated memory
char* read_file(const char* filename);

// Function to free dynamically allocated memory used for file contents
void free_file_content(char* content);

#ifdef __cplusplus
}
#endif

#endif /* FILE_READER_H */
