#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// Membaca dan menampilkan VmSize & VmRSS langsung dari /proc/self/status
void cetak_status_memori(const char *label) {
    long vmsize = 0, vmrss = 0;
    FILE *file = fopen("/proc/self/status", "r");

    if (file) {
        char baris[256];
        while (fgets(baris, sizeof(baris), file)) {
            if (strncmp(baris, "VmSize:", 7) == 0) sscanf(baris + 7, "%ld", &vmsize);
            if (strncmp(baris, "VmRSS:", 6) == 0)  sscanf(baris + 6, "%ld", &vmrss);
        }
        fclose(file);
    }
    printf("VmSize %s: %ld KB | VmRSS %s: %ld KB\n", label, vmsize, label, vmrss);
}

// Fungsi utama alokasi (1 = malloc, 2 = mmap)
void jalankan_alokasi(int pilihan) {
    printf(pilihan == 1 ? "You picked malloc/free.\n\n" : "You picked mmap/munmap.\n\n");

    cetak_status_memori("Before");

    int n;
    printf("How many integers to allocate: ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("Jumlah input tidak valid.\n\n");
        return;
    }

    size_t bytes = (size_t)n * sizeof(int);
    int *data = NULL;

    // Alokasi memori
    if (pilihan == 1) {
        data = malloc(bytes);
    } else {
        data = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (data == MAP_FAILED) data = NULL;
    }

    if (!data) {
        perror("Alokasi memori gagal");
        return;
    }

    printf("Starting memory address: %p\n", (void *)data);

    // Tulis nilai ke memori (memicu masuk ke RAM fisik / VmRSS naik)
    for (int i = 0; i < n; i++) {
        data[i] = i + 1;
    }

    // Cetak 10 angka terakhir secara terbalik
    printf("Printing first 10 values in reverse order:\n");
    int batas = (n > 10) ? n - 10 : 0;
    for (int i = n - 1; i >= batas; i--) {
        printf("%d\n", data[i]);
    }

    cetak_status_memori("After");
    printf("\n");

    // Dealokasi memori
    if (pilihan == 1) {
        free(data);
    } else {
        munmap(data, bytes);
    }
}

int main(void) {
    int input;

    while (1) {
        printf("=== KELOMPOK B07 TUGAS MEMORI ===\n");
        printf("1. Allocate with malloc/free\n");
        printf("2. Allocate with mmap/munmap\n");
        printf("3. Eksekusi Fitur Variasi Kelompok\n");
        printf("4. Exit\n");
        printf("Input: ");

        if (scanf("%d", &input) != 1) break;

        switch (input) {
            case 1:
            case 2:
                jalankan_alokasi(input);
                break;
            case 3:
                printf("Fitur variasi kelompok belum dihubungkan.\n\n");
                break;
            case 4:
                return 0;
            default:
                printf("Pilihan tidak valid.\n\n");
                break;
        }
    }

    return 0;
}
