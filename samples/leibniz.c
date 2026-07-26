// samples/leibniz.c

double leibniz_pi(int terms) {
    double pi = 0.0;
    double sign = 1.0;
    double denom = 0.0;

    for (int i = 0; i < terms; ++i) {
        denom = 2.0 * i + 1.0;
        pi += sign * 4.0 / denom;
        sign = -sign;
    }

    return pi;
}

double result = leibniz_pi(100000);
print_dbl(result);
