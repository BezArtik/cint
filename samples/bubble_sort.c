
int arr[] = { 64, 34, 25, 12, 22, 11, 90, 45 };

for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7 - i; j++) {
        if (arr[j] > arr[j + 1]) {
            int temp = arr[j];
            arr[j] = arr[j + 1];
            arr[j + 1] = temp;
        }
    }
}

for (int i = 0; i < 8; i++) {
    print(arr[i]);
}