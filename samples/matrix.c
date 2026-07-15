
// clang-format off

void print_matrix(double m[3][3], string name) {
    print_str(name);
    print_str(" = ");
    print();
    
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            print_dbl(m[i][j]);
            print_str(" ");
        }
        print();
    }
    print();
}


void add_matrices(double C[3][3], double A[3][3], double B[3][3]) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            C[i][j] = A[i][j] + B[i][j];
        }
    }
}

double A[3][3] = {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0}
};

double B[3][3] = {
    {9.0, 8.0, 7.0},
    {6.0, 5.0, 4.0},
    {3.0, 2.0, 1.0}
};

double C[3][3]; 

print_matrix(A, "A");
print_matrix(B, "B");

add_matrices(C, A, B);
print_matrix(C, "A + B");

// clang-format on
