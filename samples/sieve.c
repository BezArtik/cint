// samples/sieve.c

print_str("Enter the upper limit for prime numbers: \n");
int MAX = stoi(input());
int BITS_PER_INT = 32;

int bits[1000000];

void set_bit(int n) {
    int idx = n / BITS_PER_INT;
    int bit = n % BITS_PER_INT;
    int mask = 1 << bit;
    bits[idx] = bits[idx] | mask;
}

bool get_bit(int n) {
    int idx = n / BITS_PER_INT;
    int bit = n % BITS_PER_INT;
    int mask = 1 << bit;
    return (bits[idx] & mask) != 0;
}

int count_primes() {
    int count = 0;
    int printed = 0;

    for (int i = 2; i < MAX; ++i) {
        if (!get_bit(i)) {
            ++count;
            if (printed < 20) {
                print_int(i);
                print_str(" ");
                ++printed;
            }
        }
    }

    print();
    return count;
}

print_str("Sieve of Eratosthenes up to ");
print_int(MAX);
print_str("\n");
print_str("====================================\n");

for (int i = 2; i * i < MAX; ++i) {
    if (!get_bit(i)) {
        for (int j = i * i; j < MAX; j += i) { set_bit(j); }
    }
}

print_str("First 20 primes: ");
int total = count_primes();
print_str("Total primes: ");
print_int(total);
print_str("\n");
