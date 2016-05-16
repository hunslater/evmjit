#include <stdint.h>  // Definition of uint64_t.
#include <stddef.h>  // Definition of size_t.

/// 32 bytes of data representing host-endian 256-bit integer.
struct evmjit_uint256 {
    uint64_t words[4];
};

/// Ethereum address.
struct evmjit_hash160 {
    char bytes[20];
};

/// 32 bytes of data. For EVM that means big-endian 256-bit integer.
struct evmjit_hash256 {
    _Alignas(8) char bytes[32];
};

/// Reference to memory.
struct evmjit_bytes_view {
    char const* bytes;
    size_t size;
};

enum evmjit_return_code {
    evmjit_return = 0,
    evmjit_selfdestruct = 1,
    evmjit_exception = -1,
};

/// Complex struct representing execution result.
struct evmjit_result {
    enum evmjit_return_code return_code;  ///< Success? OOG? Selfdestruction?
    union {
        /// In case of successful execution this substruct is filled.
        struct {
            struct evmjit_bytes_view output_data;  ///< Ref to output data.
            int64_t gas;                           ///< Gas left.
            void* internal_memory;                 ///< For internal use.
        };
        /// In case of selfdestruction here is the address where money goes.
        struct evmjit_hash160 selfdestruct_beneficiary;
    };
};

enum evmjit_query_key {
    evmjit_query_gas_price,
    evmjit_query_address,
    evmjit_query_caller,
    evmjit_query_origin,
    evmjit_query_coinbase,
    evmjit_query_difficulty,
    evmjit_query_gas_limit,
    evmjit_query_number,
    evmjit_query_timestamp,
    evmjit_query_code_by_hash,  // For future use :)
    evmjit_query_code_by_address,

    evmjit_query_balance,

    evmjit_storage_load,
};


/// Opaque struct representing execution enviroment managed by the host
/// application.
struct evmjit_env;

typedef uint64_t (*evmjit_query_uint64_func)(struct evmjit_env*,
                                             enum evmjit_query_key);

typedef struct evmjit_uint256 (*evmjit_query_uint256_func)(
    struct evmjit_env*,
    enum evmjit_query_key,
    struct evmjit_uint256 arg);

typedef struct evmjit_bytes_view (
    *evmjit_query_bytes_func)(struct evmjit_env*, enum evmjit_query_key);

typedef void (*evmjit_store_storage_func)(struct evmjit_env*,
                                          struct evmjit_uint256 key,
                                          struct evmjit_uint256 value);


/// Returns EVMJIT software version.
///
/// TODO: Is int a good type? E.g. version 1.2.30 being 10230?
int evmjit_get_version();


struct evmjit_instance;

/// Creates JIT instance.
///
/// Creates new JIT instance. Each instance must be destroy in
/// evmjit_destroy_instance() function.
/// Single instance is thread-safe and can be shared by many threads. The host
/// application can create as many instances as wanted by there are no benefits
/// of this strategy as instances will not share generated code.
///
/// @params Pointers to callback functions.
///         TODO: call, create, log.
struct evmjit_instance* evmjit_create_instance(evmjit_query_uint64_func,
                                               evmjit_query_uint256_func,
                                               evmjit_query_bytes_func,
                                               evmjit_store_storage_func);

/// Destroys JIT instance.
void evmjit_destroy_instance(struct evmjit_instance*);


/// Configures a JIT instance.
///
/// Allows modifying options of a JIT instance.
/// Options:
/// - compatibility mode: frontier, homestead, metropolis, ...
/// - code cache behavior: on, off, read-only, ...
/// - optimizations,
///
/// TODO: int is probably not the best choice for params type. Would a c-string
///       as char const* be better?
/// TODO: Is there a need for evmjit_get_option()?
void evmjit_set_option(struct evmjit_instance*, int key, int value);


/// Generates and executes machine code for given EVM bytecode.
///
/// All the fun is here. This function actually does something useful.
///
/// @param instance    A JIT instance.
/// @param code_hash   A hash of the bytecode, usually Keccak. EVMJIT uses it as
///                    the code identifier. EVMJIT is able to hash the code
///                    itself, but the host application usually has the hash
///                    already.
/// @param code        Reference to the bytecode to be executed.
/// @param gas         Gas for execution. Min 0, max 2^63-1.
/// @param input_data  Reference to the call input data.
/// @param value       Call value. TODO: Does the VM need to know what kind of
///                    value it is?
/// @return            All execution results.
struct evmjit_result evmjit_execute(struct evmjit_instance* instance,
                                    struct evmjit_hash256 code_hash,
                                    struct evmjit_bytes_view code,
                                    int64_t gas,
                                    struct evmjit_bytes_view input_data,
                                    struct evmjit_uint256 value);

/// Destroys execution result.
void evmjit_destroy_result(struct evmjit_result);


/// Example how the API is supposed to be used.
void example() {
    struct evmjit_uint256 balance(struct evmjit_env*,
                                  struct evmjit_hash256 address);

    uint64_t get_uint64(struct evmjit_env * env, enum evmjit_query_key key) {
        (void)env;
        switch (key) {
        case evmjit_query_gas_price: return 1;
        default: return 0;
        }
    }

    struct evmjit_uint256 get_uint256(struct evmjit_env * env,
                                      enum evmjit_query_key key,
                                      struct evmjit_uint256 arg) {
        switch (key) {
        case evmjit_query_balance: {
            // Interpret the argument as hash/bytes/BE number.
            // EVMJIT is aware and will do the byte swap for us.
            struct evmjit_hash256 address = *(struct evmjit_hash256*)&arg;
            return balance(env, address);
        }
        default: return arg;
        }
    }

    struct evmjit_instance* jit =
        evmjit_create_instance(get_uint64, get_uint256, 0, 0);

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
