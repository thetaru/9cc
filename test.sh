#!/bin/bash

cat <<EOF | cc -xc -c -o tmp2.o -
#include <stdio.h>
#include <stdlib.h>
int test1() { return 1; }
int test2() { return 2; }
int test3(int x, int y) { return x + y; }
int test4(int x, int y) { return x - y; }
int fib(int n) { if(n==1) return 1; if(n==2) return 1; return fib(n-1) + fib(n-2); }
void alloc4(int **p, int a, int b, int c, int d) { *p = malloc(sizeof(int) * 4); (*p)[0] = a; (*p)[1] = b; (*p)[2] = c; (*p)[3] = d; }
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

assert 1 "int main() { return 1; }"

assert 6 "int main() { int a; a = 2; int b; b = 3; return a * b; }"

assert 2 "int main() { int a; a = 0 < 1; int b; b = 0 <= 1; int c; c = 0 > 1; int d; d = 0 >= 1; return a + b + c + d; }"

assert 0 "int main() { int a; a = 0; if (a == 0) { return 0; } }"

assert 1 "int main() { int a; a = 0; if (a != 0) { return 0; } else { return 1; } }"

assert 10 "int main() { int i; i = 1; while (i < 10) { i = i + 1; } return i; }"

assert 55 "int main() { int sum; sum = 0; int i; i = 1; for (i = 1; i <= 10; i = i + 1) sum = sum + i; return sum; }"

assert 10 "int main() { for (;;) { return 10; } }"

assert 1 "int main() { return test1(); }"
assert 2 "int main() { return test2(); }"
assert 3 "int main() { return test3(1, 2); }"
assert 4 "int main() { return test4(5, 1); }"
assert 5 "int main() { return fib(5); }"
assert 5 "int main() { return const(); } int const() { return 5; }"
assert 6 "int main() { int a; a = 2; int b; b = 3; return a * b; }"
assert 3 "int main() { return id(3); } int id(int x) { return 3; }"
assert 3 "int main() { int a; a = 3; return id(a); } int id(int a) { return a; }"
assert 5 "int main() { int a; a = 3; return id(a); } int id(int a) { a = 5; return a; }"
assert 3 "int main() { int a; a = 3; return id(a); } int id(int x) { return x; }"
assert 5 "int main() { int a; a = 5; return fibo(a); } int fibo(int n) { if(n==1) return 1; if(n==2) return 1; return fib(n-1) + fib(n-2); }"

assert 3 "int main() { int x; x = 3; int y; y = &x; return *y; }"
assert 3 "int main() { int x; x = 3; int y; y = 5; int z; z = &y + 8; return *z; }"

assert 3 "int main() { int x; int *y; y = &x; *y = 3; return x; }"

assert 4 "int main() { int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; return *q; }"
assert 8 "int main() { int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 2; q = p + 3; return *q; }"

assert 4 "int main() { int x; return sizeof(x); }"
assert 8 "int main() { int *x; return sizeof(x); }"
assert 4 "int main() { int *x; return sizeof(1); }"
assert 4 "int main() { int *x; return sizeof(sizeof(1)); }"
assert 4 "int main() { int *x; return sizeof(*x); }"
assert 8 "int main() { int *x; return sizeof(x+1); }"
assert 8 "int main() { int *x; return sizeof(1+x); }"
assert 8 "int main() { int *x; int y; y = 1; return sizeof(x+y); }"
#assert 8 "int main() { int *x; int y; y = 1; return sizeof(y+x); }"
assert 8 "int main() { int **x; return sizeof(*x+1); }"
assert 4 "int main() { int **x; return sizeof(**x+1); }"

assert 0 "int main() { int a[5]; return 0; }"

assert 3 "int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }"


echo OK
