#include "evmjit.h"

int evmjit_get_version()
{
    int v[] = {0, 9, 0};
    return v[0] * 10000 + v[1] * 100 + v[0];
}
