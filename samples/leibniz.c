// samples/leibniz.c

double leibniz_pi(int terms) {
    double pi = 0.0;
    int i = 0;
    double sign = 1.0;
    double denom = 0.0;

    while (i < terms) {
        denom = 2.0 * i + 1.0;
        pi = pi + sign * 4.0 / denom;
        sign = 0.0 - sign;
        i = i + 1;
    }

    return pi;
}

double result = leibniz_pi(100000);
print_dbl(result);
