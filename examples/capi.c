#include "evmjit.h"

struct evmjit_uint256 balance(struct evmjit_env*,
                              struct evmjit_hash160 address);

union evmjit_variant query(struct evmjit_env* env,
                           enum evmjit_query_key key,
                           union evmjit_variant arg) {
    union evmjit_variant result;
    switch (key) {
    case evmjit_query_gas_limit: result.uint64 = 314; break;

    case evmjit_query_balance:
        result.uint256 = balance(env, arg.address);
        break;

    default: result.uint64 = 0; break;
    }
    return result;
}

/// Example how the API is supposed to be used.
void example() {
    struct evmjit_instance* jit = evmjit_create_instance(query, 0, 0, 0);

    char const code[] = "exec()";
    struct evmjit_bytes_view code_view = {code, sizeof(code)};
    struct evmjit_hash256 code_hash = {{1, 2, 3}};
    struct evmjit_bytes_view input = {"Hello World!", 12};
    struct evmjit_uint256 value = {{1, 0, 0, 0}};

    int64_t gas = 200000;
    struct evmjit_result result =
        evmjit_execute(jit, code_hash, code_view, gas, input, value);

    evmjit_destroy_result(result);
    evmjit_destroy_instance(jit);
}
