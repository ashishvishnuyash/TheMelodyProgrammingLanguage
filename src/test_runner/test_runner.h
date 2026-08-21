#ifndef MELODY_TEST_RUNNER_H
#define MELODY_TEST_RUNNER_H

/* Run tests in the given path (file or directory).
   Returns 0 if all pass, 1 if any fail. */
int test_runner_run(const char* path);

#endif
