#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>  // Untuk system call open()
#include <unistd.h> // Untuk system call write() & close()
#include <time.h>   // Untuk timestamp nama file log

#define KELOMPOK "B07"

// Variabel penyimpan metrik sementara untuk keperluan pencatatan log
static long log_vmsize_last = 0;
static long log_vmrss_last = 0;
long page_size;
// Membaca dan menampilkan VmSize & VmRSS langsung dari /proc/self/status
void cetak_status_memori(const char *label)
{
    long vmsize = 0, vmrss = 0;
    FILE *file = fopen("/proc/self/status", "r");

    if (file)
    {
        char baris[256];
        while (fgets(baris, sizeof(baris), file))
        {
            if (strncmp(baris, "VmSize:", 7) == 0)
                sscanf(baris + 7, "%ld", &vmsize);
            if (strncmp(baris, "VmRSS:", 6) == 0)
                sscanf(baris + 6, "%ld", &vmrss);
        }
        fclose(file);
    }
    printf("VmSize %s: %ld KB | VmRSS %s: %ld KB\n", label, vmsize, label, vmrss);

    // Simpan nilai terakhir untuk fungsi log
    log_vmsize_last = vmsize;
    log_vmrss_last = vmrss;
}

// Mencatat data hasil pengujian memori ke berkas log
void tulis_log_std(const char *nama_file, int pilihan, long vmsize_bef, long vmrss_bef,
                   void *addr, int *data, int n, long vmsize_aft, long vmrss_aft)
{
    FILE *fp = fopen(nama_file, "w");
    if (!fp)
    {
        perror("Gagal membuka file log std");
        return;
    }

    fprintf(fp, "%s\n", pilihan == 1 ? "You picked malloc/free." : "You picked mmap/munmap.");
    fprintf(fp, "VmSize Before: %ld KB | VmRSS Before: %ld KB\n", vmsize_bef, vmrss_bef);
    fprintf(fp, "Starting memory address: %p\n", addr);
    fprintf(fp, "Printing first 10 values in reverse order:\n");

    int batas = (n > 10) ? n - 10 : 0;
    for (int i = n - 1; i >= batas; i--)
    {
        fprintf(fp, "%d\n", data[i]);
    }

    fprintf(fp, "VmSize After: %ld KB | VmRSS After: %ld KB\n", vmsize_aft, vmrss_aft);
    fclose(fp);
}

// Mencatat data hasil pengujian memori ke berkas log menggunakan System Call
void tulis_log_sys(const char *nama_file, int pilihan, long vmsize_bef, long vmrss_bef,
                   void *addr, int *data, int n, long vmsize_aft, long vmrss_aft)
{
    int fd = open(nama_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("Gagal membuka file log sys");
        return;
    }

    char buf[2048];
    int len = 0;

    len += snprintf(buf + len, sizeof(buf) - len, "%s\n",
                    pilihan == 1 ? "You picked malloc/free." : "You picked mmap/munmap.");
    len += snprintf(buf + len, sizeof(buf) - len,
                    "VmSize Before: %ld KB | VmRSS Before: %ld KB\n", vmsize_bef, vmrss_bef);
    len += snprintf(buf + len, sizeof(buf) - len,
                    "Starting memory address: %p\n", addr);
    len += snprintf(buf + len, sizeof(buf) - len,
                    "Printing first 10 values in reverse order:\n");

    int batas = (n > 10) ? n - 10 : 0;
    for (int i = n - 1; i >= batas; i--)
    {
        len += snprintf(buf + len, sizeof(buf) - len, "%d\n", data[i]);
    }

    len += snprintf(buf + len, sizeof(buf) - len,
                    "VmSize After: %ld KB | VmRSS After: %ld KB\n", vmsize_aft, vmrss_aft);

    if (write(fd, buf, len) < 0)
    {
        perror("Gagal menulis file log sys");
    }

    close(fd);
}

// Membuat berkas log
void catat_log(int pilihan, long vmsize_bef, long vmrss_bef,
               void *addr, int *data, int n, long vmsize_aft, long vmrss_aft)
{
    const char *metode = (pilihan == 1) ? "malloc" : "mmap";
    long ts = (long)time(NULL);

    char nama_std[128], nama_sys[128];
    snprintf(nama_std, sizeof(nama_std), "%s-%s_std-%ld.txt", KELOMPOK, metode, ts);
    snprintf(nama_sys, sizeof(nama_sys), "%s-%s_sys-%ld.txt", KELOMPOK, metode, ts);

    tulis_log_std(nama_std, pilihan, vmsize_bef, vmrss_bef, addr, data, n, vmsize_aft, vmrss_aft);
    tulis_log_sys(nama_sys, pilihan, vmsize_bef, vmrss_bef, addr, data, n, vmsize_aft, vmrss_aft);
}

// Fungsi utama alokasi (1 = malloc, 2 = mmap)
void jalankan_alokasi(int pilihan)
{
    printf(pilihan == 1 ? "You picked malloc/free.\n\n" : "You picked mmap/munmap.\n\n");

    cetak_status_memori("Before");
    long vmsize_bef = log_vmsize_last;
    long vmrss_bef = log_vmrss_last;

    int n;
    printf("How many integers to allocate: ");
    if (scanf("%d", &n) != 1 || n <= 0)
    {
        printf("Jumlah input tidak valid.\n\n");
        return;
    }

    size_t bytes = (size_t)n * sizeof(int);
    int *data = NULL;

    // Alokasi memori
    if (pilihan == 1)
    {
        data = malloc(bytes);
    }
    else
    {
        data = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (data == MAP_FAILED)
            data = NULL;
    }

    if (!data)
    {
        perror("Alokasi memori gagal");
        return;
    }

    printf("Starting memory address: %p\n", (void *)data);

    // Tulis nilai ke memori (memicu masuk ke RAM fisik / VmRSS naik)
    for (int i = 0; i < n; i++)
    {
        data[i] = i + 1;
    }

    // Cetak 10 angka terakhir secara terbalik
    printf("Printing first 10 values in reverse order:\n");
    int batas = (n > 10) ? n - 10 : 0;
    for (int i = n - 1; i >= batas; i--)
    {
        printf("%d\n", data[i]);
    }

    cetak_status_memori("After");
    long vmsize_aft = log_vmsize_last;
    long vmrss_aft = log_vmrss_last;
    printf("\n");

    // Panggil fungsi log sebelum memori di-free / munmap
    catat_log(pilihan, vmsize_bef, vmrss_bef, (void *)data, data, n, vmsize_aft, vmrss_aft);

    // Dealokasi memori
    if (pilihan == 1)
    {
        free(data);
    }
    else
    {
        munmap(data, bytes);
    }
}

void jalankan_variasi_kelompok(){
    int *arr = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);    
    mprotect(arr, page_size, PROT_READ);
    printf("--- Varian 7: mprotect & Signal ---\n");
    printf("[Output] Mencoba menulis ke memori Read-Only...\n");
    arr[0] = 1;
    printf("[Output] Berhasil menulis setelah signal handler dipanggil\n");
}
static void handler(int sig, siginfo_t *info, void *unused)
{
    if (info->si_code != SEGV_ACCERR) { // makes sure signal is caused by forbidden write
        _exit(EXIT_FAILURE);
    }
    char msg[] = "[Signal Handler] SIGSEGV tertangkap! Memulihkan isin PROT_WRITE...\n";
    write(1, msg, sizeof(msg) - 1);
    uintptr_t page_start = (uintptr_t)info->si_addr & ~((uintptr_t)page_size - 1); // rounds memory address down to page start to allign with mprotect
    if(mprotect((void *)page_start, page_size, PROT_READ | PROT_WRITE) == -1){
        _exit(EXIT_FAILURE);
    }
}
int main(void)
{
    page_size = sysconf(_SC_PAGESIZE);
    int input;
    struct sigaction sa;
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) < 0) {
        perror("sigaction registration failed");
        return EXIT_FAILURE;
    }

    while (1)
    {
        printf("=== KELOMPOK B07 TUGAS MEMORI ===\n");
        printf("1. Allocate with malloc/free\n");
        printf("2. Allocate with mmap/munmap\n");
        printf("3. Eksekusi Fitur Variasi Kelompok\n");
        printf("4. Exit\n");
        printf("Input: ");

        if (scanf("%d", &input) != 1)
            break;

        switch (input)
        {
        case 1:
        case 2:
            jalankan_alokasi(input);
            break;
        case 3:
            jalankan_variasi_kelompok();
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