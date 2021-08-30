#!/bin/bash

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 1 "main() { return 1; }"

assert 6 "main() { a = 2; b = 3; return a * b; }"

assert 2 "main() { a = 0 < 1; b = 0 <= 1; c = 0 > 1; d = 0 >= 1; return a + b + c + d; }"

assert 0 "main() { a = 0; if (a == 0) { return 0; } }"

assert 1 "main() { a = 0; if (a != 0) { return 0; } else { return 1; } }"

assert 10 "main() { i = 1; while (i < 10) i = i + 1; return i; }"

assert 55 "main() { sum = 0; for (i = 1; i <= 10; i = i + 1) sum = sum + i; return sum; }"

echo OK
