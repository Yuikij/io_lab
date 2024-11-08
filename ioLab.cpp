#include <stdio.h>
#include <stdint.h>

#define SEXT(x, len) ({ struct { int64_t n : len; } __x = { .n = x }; (uint64_t)__x.n; })
// void print_binary(int64_t x);
int main()
{
    int x = 0b101;
    int len = 8;
    int64_t y = SEXT(x, 20);
    // for (int i = 63; i >= 0; i--)
    // {
    //     printf("%d", (x >> i) & 1);
    // }

    printf("%s", "test123");
     printf("\n");
    // for (int i = 63; i >= 0; i--)
    // {
    //     printf("%d", (y >> i) & 1);
    // }
    // print_binary(y);
    return 0;
}

// void print_binary(int64_t x) {
//     for (int i = 63; i >= 0; i--) {
//         printf("%d", (x >> i) & 1);
//         if (i % 4 == 0) {
//             printf(" ");
//         }
//     }
//     printf("\n");
// }
