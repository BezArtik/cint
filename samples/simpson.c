// samples/simpson.c

double PI = 3.141592653589793;

double f(double x) {
    return sin(x) * exp(-x / 2.0);
}

double simpson(double a, double b) {
    double mid = (a + b) / 2.0;
    return (b - a) / 6.0 * (f(a) + 4.0 * f(mid) + f(b));
}

double adaptive(double a, double b, double eps, int depth) {
    double mid = (a + b) / 2.0;
    double whole = simpson(a, b);
    double left = simpson(a, mid);
    double right = simpson(mid, b);
    double diff = left + right - whole;

    if (diff < 0.0) { diff = 0.0 - diff; }

    if (diff < 15.0 * eps || depth > 20) { return left + right + (left + right - whole) / 15.0; }

    return adaptive(a, mid, eps / 2.0, depth + 1) + adaptive(mid, b, eps / 2.0, depth + 1);
}

double eps = 0.000001;

print_str("Integral of sin(x)*e^(-x/2) from 0 to PI\n");
print_str("============================================\n\n");

int powers = 6;
int i = 0;

while (i < powers) {
    double result = adaptive(0.0, PI, eps, 0);

    print_str("eps = ");
    print_dbl(eps);
    print_str("\n");
    print_str("result = ");
    print_dbl(result);
    print_str("\n");

    eps = eps / 10.0;
    double exact = 4.0 / 5.0 * (1.0 + exp(-PI / 2.0));
    print_str("Exact value: ");
    print_dbl(exact);
    print_str("\n");
    print_str("Error: ");
    print_dbl(result - exact);
    print_str("\n");
    i++;
}
