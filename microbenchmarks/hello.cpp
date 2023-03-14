#include<iostream>
#include <unistd.h>

using namespace std;

void hello_stack() {
    int stack_i = 1;
    int stack_j = 2;
    int stack_k = 3;
    printf("%-20s %016lx\n", "stack_i", (uint64_t) &stack_i);
    printf("%-20s %016lx\n", "stack_j", (uint64_t) &stack_j);
    printf("%-20s %016lx\n", "stack_k", (uint64_t) &stack_k);
}

int main()
{
    cout<<"Hello World"<<endl;
    uint32_t* p = (uint32_t *) malloc(4);
    uint32_t* q = (uint32_t *) malloc(4);
    uint32_t* r = (uint32_t *) malloc(4);
    //uint32_t* t = (uint32_t *) calloc(10, 4);
    *p = 0xdeadbeef;
    *q = 0xabcd1234;
    *r = 0xfafa5555;
    *q = 0xcafecafe;
    printf("p addr=0x%016lx\n", (uint64_t) p);
    printf("q addr=0x%016lx\n", (uint64_t) q);
    printf("r addr=0x%016lx\n", (uint64_t) r);
   // printf("t addr=0x%016lx\n", (uint64_t) t);
    //usleep(5000000);
    printf("p        = 0x%08x\n", *p);
    printf("q        = 0x%08x\n", *q);
    printf("r        = 0x%08x\n", *r);
    //printf("t        = 0x%08x\n", *t);
    q = (uint32_t *) realloc((void*) q, 64);
    printf("q addr (realloc)=0x%016lx\n", (uint64_t) q);
    printf("q (realloc)     = 0x%08x\n", *q);
    if (*q != 0xcafecafe) printf("ERROR: q value mismatch after realloc\n");
    //*q = 0xcafecafe;
    //printf("q        = 0x%08x\n", *q);
    free(p);
    free(q);
    free(r);
    //free(t);

    hello_stack();
    return 0;
}
