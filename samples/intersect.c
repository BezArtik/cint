// samples/intersect.c

struct Point {
    double x;
    double y;
};

struct Circle {
    struct Point center;
    double radius;
};

struct Pair {
    int first;
    int second;
};

int MAX = 500;
int BITS_PER_INT = 32;
int bits[10000];

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

struct Circle circles[500];

double distance(struct Point a, struct Point b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}

bool intersect(struct Circle c1, struct Circle c2) {
    double d = distance(c1.center, c2.center);
    double sum = c1.radius + c2.radius;
    double diff = c1.radius - c2.radius;
    if (diff < 0) { diff = -diff; }
    return d <= sum && d >= diff;
}

int count_intersections() {
    int count = 0;
    for (int i = 0; i < MAX; ++i) {
        for (int j = i + 1; j < MAX; ++j) {
            if (intersect(circles[i], circles[j])) { ++count; }
        }
    }
    return count;
}

print_str("Generating ");
print_int(MAX);
print_str(" random circles...\n");

srand(42);

for (int i = 0; i < MAX; ++i) {
    circles[i].center.x = itod(rand_int(0, 1000));
    circles[i].center.y = itod(rand_int(0, 1000));
    circles[i].radius = rand_dbl(10.0, 100.0);
}

print_str("Counting intersections...\n");

double start_time = itod(0);

int result = count_intersections();

print_str("Number of intersecting pairs: ");
print_int(result);
print_str("\n");

struct Point p1;
p1.x = 0.0;
p1.y = 0.0;

struct Point p2;
p2.x = 3.0;
p2.y = 4.0;

double d = distance(p1, p2);
print_str("Distance from (0,0) to (3,4): ");
print_dbl(d);
print_str("\n");

struct Circle unit_circle;
unit_circle.center.x = 0.0;
unit_circle.center.y = 0.0;
unit_circle.radius = 5.0;

struct Circle test_circle;
test_circle.center.x = 3.0;
test_circle.center.y = 4.0;
test_circle.radius = 1.0;

print_str("Unit circle at (0,0) r=5, Test circle at (3,4) r=1: ");
if (intersect(unit_circle, test_circle)) {
    print_str("intersect");
} else {
    print_str("do not intersect");
}
print_str("\n");
