

int monte_carlo_pi(int iterations) {
    int inside = 0;
    int i = 0;
    int seed = 1;
    double x = 0.0;
    double y = 0.0;

    while (i < iterations) {
        seed = (seed * 1103515245 + 12345) % 2147483648;
        x = seed;
        x = x / 1073741824.0 - 1.0;

        seed = (seed * 1103515245 + 12345) % 2147483648;
        y = seed;
        y = y / 1073741824.0 - 1.0;

        if (x * x + y * y <= 1.0) { inside = inside + 1; }

        i = i + 1;
    }

    double pi = 4.0 * inside / iterations;
    print(pi);
    return inside;
}

int result = monte_carlo_pi(100000);
