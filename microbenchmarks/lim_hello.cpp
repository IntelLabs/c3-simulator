#include<iostream>
#include <unistd.h>

using namespace std;

int main()
{
    cout<<"Hello World"<<endl;
    int size = 48;
    uint8_t* p_enc = (uint8_t *) malloc(size);
    printf("p_enc addr    = 0x%016lx\n", (uint64_t) p_enc);

    for (int i = 0; i < size; i++) {
        p_enc[i] = (uint8_t) i;
    }
    for (int i = 0; i < size; i++) {
        printf("*0x%016lx = 0x%02x\n", (uint64_t)&p_enc[i], p_enc[i]);
    }

    free(p_enc);
    return 0;
}
