

int fib(int n) {
    if (n < 2) {
        return n;
    }
    int a = 0;
    int b = 1;
    int i = 2;
    while (i <= n) {
        int t = a + b;
        a = b;
        b = t;
        i = i + 1;
    }
    return b;
}

int i = 0;
while (i < 20) {
    print(fib(i));
    i = i + 1;
}