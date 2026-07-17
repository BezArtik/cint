// samples/bubble_sort.c

int arr[1000];
int size = 1000;

for (int i = 0; i < size; ++i) { arr[i] = rand_int(1, size); }

for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - 1 - i; j++) {
        if (arr[j] > arr[j + 1]) {
            int temp = arr[j];
            arr[j] = arr[j + 1];
            arr[j + 1] = temp;
        }
    }
}

for (int i = 0; i < 10; i++) {
    print_int(arr[i]);
    print();
}
