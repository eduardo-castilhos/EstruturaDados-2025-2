void tres_loops(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                arr[i] = arr[i] + arr[j] + arr[k];
            }
        }
    }
}
