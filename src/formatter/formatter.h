#ifndef MELODY_FORMATTER_H
#define MELODY_FORMATTER_H

/* Format a single .mdy file in place. Returns 0 on success, -1 on error. */
int formatter_format_file(const char* path);

/* Format all .mdy files in a directory recursively. Returns count of failures. */
int formatter_format_dir(const char* dir_path);

#endif
