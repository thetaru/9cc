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

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 10 "-10+20;"
assert 10 "- -10;"
assert 10 "- - +10;"

assert 0 "0==1;"
assert 1 "42==42;"
assert 1 "0!=1;"
assert 0 "42!=42;"

assert 1 "0<1;"
assert 0 "1<1;"
assert 0 "2<1;"
assert 1 "0<=1;"
assert 1 "1<=1;"
assert 0 "2<=1;"

assert 1 "1>0;"
assert 0 "1>1;"
assert 0 "1>2;"
assert 1 "1>=0;"
assert 1 "1>=1;"
assert 0 "1>=2;"

assert 14 "a = 3; b = 5 * 6 - 8; a + b / 2;"

assert 6 "foo = 1; bar = 2 + 3; foo + bar;"

assert 14 "a = 3; b = 5 * 6 - 8; return a + b / 2;"
assert 5 "return 5;"
assert 8 "return 8;"

# IF
assert 1 "a = 1; if (a == 1) return a;"

# IF-ELSE
assert 0 "a = 1; if (a == 1) return 0; else return 1;"
assert 1 "a = 1; if (a != 1) return 0; else return 1;"

# WHILE
assert 10 "i = 1; while (i < 10) i = i + 1; return i;"

# FOR
assert 55 "sum = 0; for (i = 1; i <= 10; i = i + 1) sum = sum + i; return sum;"

# BLOCK
assert 55 "sum = 0; for (i = 1; i <= 10; i = i + 1) {sum = sum + i;} return sum;"
assert 0  "a = 1; if (a == 1) { return 0; } else { return 1; }"

echo OK
