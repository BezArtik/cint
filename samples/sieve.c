// samples/sieve.c

int MAX = 1000000;
int BITS_PER_INT = 32;

int bits[31250];

void set_bit(int n) {
    int idx = n / BITS_PER_INT;
    int bit = n % BITS_PER_INT;

    int mask = 1;
    int i = 0;
    while (i < bit) {
        mask = mask * 2;
        i++;
    }

    bits[idx] = bits[idx] | mask;
}

bool get_bit(int n) {
    int idx = n / BITS_PER_INT;
    int bit = n % BITS_PER_INT;

    int mask = 1;
    int i = 0;
    while (i < bit) {
        mask = mask * 2;
        i = i + 1;
    }

    return (bits[idx] & mask) != 0;
}

int count_primes() {
    int count = 0;
    int printed = 0;

    int i = 2;
    while (i < MAX) {
        if (!get_bit(i)) {
            count = count + 1;
            if (printed < 20) {
                print_int(i);
                print_str(" ");
                printed = printed + 1;
            }
        }
        i = i + 1;
    }

    print();
    return count;
}

print_str("Sieve of Eratosthenes up to ");
print_int(MAX);
print();
print_str("====================================");
print();

int i = 2;
while (i * i < MAX) {
    if (!get_bit(i)) {
        int j = i * i;
        while (j < MAX) {
            set_bit(j);
            j = j + i;
        }
    }
    i = i + 1;
}

print_str("First 20 primes: ");
int total = count_primes();
print_str("Total primes: ");
print_int(total);
print();
