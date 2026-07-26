// samples/bubble_sort.c

int arr[1000];
int size = 1000;

print_str("Generating...\n");
for (int i = 0; i < size; ++i) { arr[i] = rand_int(1, 100000); }

print_str("Sorting...\n");
for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - 1 - i; j++) {
        if (arr[j] > arr[j + 1]) {
            int temp = arr[j];
            arr[j] = arr[j + 1];
            arr[j + 1] = temp;
        }
    }
}

print_str("Checking...\n");
bool check() {
    for (int i = 0; i < size - 1; ++i) {
        if (arr[i] > arr[i + 1]) { return false; }
    }
    return true;
}
bool res = check();

if (res) {
    print_str("The array is sorted\n");
} else {
    print_str("The array is not sorted\n");
}
