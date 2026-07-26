// samples/fib.c

int fib(int n) {
    if (n < 2) { return n; }
    int a = 0;
    int b = 1;
    for (int i = 2; i <= n; ++i) {
        int t = a + b;
        a = b;
        b = t;
    }
    return b;
}

print_str("Fibonacci numbers: \n");
for (int i = 0; i < 20; ++i) {
    print_int(fib(i));
    print_str("\n");
}
