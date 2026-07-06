// samples/prime.c

bool is_prime(int n) {
    if (n <= 1) { return false; }
    if (n <= 3) { return true; }
    if (n % 2 == 0 || n % 3 == 0) { return false; }
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) { return false; }
    }
    return true;
}

int count = 0;
print_str("Enter the upper limit for prime numbers: ");
int n = stoi(input());
for (int i = 2; i <= n; ++i) {
    bool prime = is_prime(i);
    if (prime) {
        print_int(i);
        ++count;
    }
}
print_str("total:");
print_int(count);
