#!/bin/bash

cat <<EOF | cc -xc -c -o tmp2.o -
int test1() { return 1; }
int test2() { return 2; }
int test3(int x, int y) { return x + y; }
int test4(int x, int y) { return x - y; }
int fib(int n) { if(n==1) return 1; if(n==2) return 1; return fib(n-1) + fib(n-2); }
EOF

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
    echo ""
}

assert 1 "main() { return 1; }"

assert 6 "main() { int a; a = 2; int b; b = 3; return a * b; }"

assert 2 "main() { int a; a = 0 < 1; int b; b = 0 <= 1; int c; c = 0 > 1; int d; d = 0 >= 1; return a + b + c + d; }"

assert 0 "main() { int a; a = 0; if (a == 0) { return 0; } }"

assert 1 "main() { int a; a = 0; if (a != 0) { return 0; } else { return 1; } }"

assert 10 "main() { int i; i = 1; while (i < 10) { i = i + 1; } return i; }"

assert 55 "main() { int sum; sum = 0; int i; i = 1; for (i = 1; i <= 10; i = i + 1) sum = sum + i; return sum; }"

assert 10 "main() { for (;;) { return 10; } }"

assert 1 "main() { return test1(); }"
assert 2 "main() { return test2(); }"
assert 3 "main() { return test3(1, 2); }"
assert 4 "main() { return test4(5, 1); }"
assert 5 "main() { return fib(5); }"
assert 5 "main() { return const(); } const() { return 5; }"
assert 6 "main() { int a; a = 2; int b; b = 3; return a * b; }"
assert 3 "main() { return id(3); } id(x) { return 3; }"
assert 3 "main() { int a; a = 3; return id(a); } id(a) { return a; }"
assert 5 "main() { int a; a = 3; return id(a); } id(a) { int a; a = 5; return a; }"
assert 3 "main() { int a; a = 3; return id(a); } id(x) { return x; }"
assert 5 "main() { int a; a = 5; return fibo(a); } fibo(n) { if(n==1) return 1; if(n==2) return 1; return fib(n-1) + fib(n-2); }"

assert 3 "main() { int x; x = 3; int y; y = &x; return *y; }"
assert 3 "main() { int x; x = 3; int y; y = &x; return *y; }"
assert 3 "main() { int x; x = 3; int y; y = 5; int z; z = &y + 8; return *z; }"

echo OK
