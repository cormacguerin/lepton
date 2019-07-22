#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

const unsigned int MOD_ADLER = 31;

unsigned int adler32(unsigned char *data, size_t len) 
{
    uint32_t a = 1, b = 0;
    size_t index;
    
    // Process each byte of the data in order
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        unsigned char *s = NULL;
        s = (unsigned char*)argv[1];
        int slen = strlen((char*)s);
        int r = adler32(s, slen);
        char str[1];
        printf("%d",r);
        printf("%s"," ");
        printf("%s",s);
        printf("%s","\n");
    }
}
