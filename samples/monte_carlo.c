// samples/monte_carlo.c

double monte_carlo_pi(int iterations) {
    int inside = 0;
    int seed = 1;
    double x = 0.0;
    double y = 0.0;

    for (int i = 0; i < iterations; ++i) {
        seed = (seed * 1103515245 + 12345) % 2147483648;
        x = itod(seed);
        x = x / 1073741824.0 - 1.0;

        seed = (seed * 1103515245 + 12345) % 2147483648;
        y = itod(seed);
        y = y / 1073741824.0 - 1.0;

        if (x * x + y * y <= 1.0) { ++inside; }
    }

    return 4.0 * inside / iterations;
}

double result = monte_carlo_pi(100000);
print_dbl(result);
