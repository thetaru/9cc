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

run() {
    ./9cc ./tests  > tmp.s
    cc -static -o tmp tmp.s tmp2.o
    ./tmp
}

run
