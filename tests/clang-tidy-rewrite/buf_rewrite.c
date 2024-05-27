#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct my_struct {
	long a;
	char b[16];
	long c;
	char d[16];
	long e;
} my_struct_t;

typedef struct my_struct_opt {
	long a;
	char b[16];
	char d[16];
        long c;
	long e;
} my_struct_opt_t;

bool structs_equal(my_struct_t *a, my_struct_opt_t *b) {
	bool non_buf_fields = a->a == b->a && a->c == b->c && a->e == b->e;
	bool buf_fields = (strncmp(a->b, b->b, sizeof(a->b)) + strncmp(a->d, b->d, sizeof(a->d))) == 0;
	return non_buf_fields && buf_fields;
}

my_struct_t *init_my_struct() {
  my_struct_t *s = malloc(sizeof(my_struct_t));
  strcpy(s->b, "Hello World!");
  strcpy(s->d, "Goodbye World!");
  s->a = 1;
  s->c = 2;
  s->e = 3;
  return s;
}

my_struct_opt_t *init_my_struct_opt() {
  my_struct_opt_t *s = malloc(sizeof(my_struct_opt_t));
  strcpy(s->b, "Hello World!");
  strcpy(s->d, "Goodbye World!");
  s->a = 1;
  s->c = 2;
  s->e = 3;
  return s;
}


int main() {
  my_struct_t *s1 = init_my_struct();
  my_struct_opt_t *s1_opt = init_my_struct_opt();

  // Check that my_struct and my_struct_ok are equal.
  // This makes sure that the clang-tidy rewrite doesn't introduce
  // incompatibilities.
  assert(structs_equal(s1, s1_opt) && "The structs are not equal. Most likely a clang-tidy rewrite error.");

  printf("Doing buffer overread now.\n");
  // Do buffer overread
  char leak = s1->b[16];

  printf("Doing another buffer overread now.\n");
  char leak2 = s1->d[19];

  printf("Doing buffer overwrite now.\n");
  s1->b[16] = 'a';
  printf("Doing another buffer overwrite now.\n");
  s1->d[19] = 'b';

  printf("TEST DONE. Should not reach here with tripwire mitigations enabled.\n");

  return 0;
}
