#ifndef MATH_FUNCTIONS_HEADER
#define MATH_FUNCTIONS_HEADER

#include <inttypes.h>

static const ssize_t MATH_FUNC_COUNT = 5;

int8_t add(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a + b;
    if (!((a ^ b) < 0) && (a ^ result) < 0) {
        errno = EOVERFLOW;
        return -1;
    }
    return 0;

}

int8_t subtract(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a - b;
    if ((a ^ b) < 0 && (a ^ result) < 0){
        errno = EOVERFLOW;
        return -1;
    }
    return 0;
}

int8_t multiply(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a * b;
    if ((a == INT64_MIN && b == -1) || (a != 0 && b != 0 && a != result / b)) {
        errno = EOVERFLOW;
        return -1;
    }
    return 0;
}

int8_t div(const int64_t& a, const int64_t& b, int64_t& result) {
    if (b == 0 || (a == INT64_MIN && b == -1)) { 
        errno = EINVAL;
        return -1; 
    }
    result = a / b;
    return 0;
}

int8_t mod(const int64_t& a, const int64_t& b, int64_t& result) {
    if (b == 0 || (a == INT64_MIN && b == -1)) { return -1; }
    result = a % b;
    return 0;
}

int8_t (*math_fun[])(const int64_t&, const int64_t&, int64_t&) = {
    add,
    subtract,
    multiply,
    div,
    mod
};

#endif