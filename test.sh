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
assert 35 "5*7;"
assert 52 "6 * 8 + 8 / 4 + 2;"
assert 10 "-10+20;"
assert 10 "- -10;"
assert 10 "- - +10;"
assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'
assert 14 'a=3;
b=5*6-8;
return a+b/2;'
assert 5 'foo = 1;
bar = 2 + 3;'
assert 5 'return 5;
return 8;'
assert 2 'foo = 1;
if(1) foo = 2;
return foo;'
assert 5 'foo = 1;
if(foo == 2) foo = 2;
else foo = 5;
return foo;'
assert 5 'foo = 1;
while(foo < 5) foo = foo + 1;
return foo;'
assert 4 'foo = 1;
for(i=0;i<3;i=i+1) foo = foo + 1;
return foo;'
assert 10 'a = 0;
for(;a<10;) a = a+1;
return a;'

assert 6 'a = 3;
if(a > 1) if(a < 4) return 6;
else return 5;'

assert 10 '
a = 0;
for(;;) {
  a = a+1;
  if(a == 5) return 10;
}'

echo OK
