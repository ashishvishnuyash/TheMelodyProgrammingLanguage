#include <stdio.h>
#include <stdlib.h>

void* data(void* arg) {
    void* result = NULL;
    result = malloc(sizeof(int));
    *((int*)result) =(*(int*)arg)+1;
    return result;

}
int main(int argc, char const *argv[]) {
   int i =5;
   void* ptr = data(&i);

  // Check if data function returned a valid pointer
  if (ptr == NULL) {
    fprintf(stderr, "Error: data() failed\n");
    return 1; // Exit with an error code
  }

  // Access the stored integer value safely
  int value = *((int*)ptr);

  printf("Value: %d\n", value);

  // Free the allocated memory (important to prevent memory leaks)
  free(ptr);

  return 0;
}