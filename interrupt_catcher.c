#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define MAX_INTERRUPTS 256

typedef struct {
    int irq;
    char name[64];
    unsigned long long count;
} InterruptInfo;

volatile sig_atomic_t running = 1;

static inline uint64_t get_system_time() {
    uint64_t time;
    asm volatile("mrs %0, cntvct_el0" : "=r" (time));
    return time;
}

static inline void get_cpu_info(char* vendor, char* brand) {
    uint64_t midr;
    asm volatile("mrs %0, midr_el1" : "=r" (midr));
    
    uint8_t implementer = (midr >> 24) & 0xFF;
    uint8_t variant = (midr >> 20) & 0xF;
    uint8_t architecture = (midr >> 16) & 0xF;
    uint16_t part_num = (midr >> 4) & 0xFFF;
    
    sprintf(vendor, "ARM%02X%01X%01X%03X", implementer, variant, architecture, part_num);
    strcpy(brand, "ARM Processor");
}

int cpu_hv() {
    uint64_t id_aa64pfr0;
    asm volatile("mrs %0, id_aa64pfr0_el1" : "=r" (id_aa64pfr0));
    return ((id_aa64pfr0 >> 40) & 0xF) != 0 ? TRUE : FALSE;
}

void cpu_write_vendor(char* vendor) {
    char brand[49];
    get_cpu_info(vendor, brand);
}

void read_interrupts(InterruptInfo *interrupts, int *count) {
    FILE *fp = fopen("/proc/interrupts", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/interrupts");
        exit(1);
    }

    char line[256];
    *count = 0;

    // Skip the header line
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) && *count < MAX_INTERRUPTS) {
        InterruptInfo *info = &interrupts[*count];
        char *token = strtok(line, " ");
        if (token == NULL) continue;

        // Remove colon from IRQ number
        char *colon = strchr(token, ':');
        if (colon) *colon = '\0';

        info->irq = atoi(token);

        // Skip CPU-specific counts
        while ((token = strtok(NULL, " ")) != NULL) {
            if (strspn(token, "0123456789") != strlen(token)) {
                break;
            }
        }

        if (token == NULL) continue;

        strncpy(info->name, token, sizeof(info->name) - 1);
        info->name[sizeof(info->name) - 1] = '\0';

        info->count = 0;
        while ((token = strtok(NULL, " ")) != NULL) {
            if (strspn(token, "0123456789") == strlen(token)) {
                info->count += strtoull(token, NULL, 10);
            }
        }

        (*count)++;
    }

    fclose(fp);
}

void signal_handler(int signum) {
    running = 0;
}

void generate_interrupts() {
    // Generate disk I/O interrupt
    FILE *file = fopen("/tmp/test_file", "w");
    if (file != NULL) {
        fprintf(file, "Test data for interrupt generation");
        fclose(file);
        remove("/tmp/test_file");
    }

    // Generate network-related interrupt (this might not work in all environments)
    system("ping -c 1 localhost &>/dev/null");

    // Generate timer interrupt
    usleep(1000);
}

int main() {
    char vendor[13] = {0};
    InterruptInfo interrupts_prev[MAX_INTERRUPTS], interrupts_curr[MAX_INTERRUPTS];
    int count_prev, count_curr;

    signal(SIGINT, signal_handler);

    printf("ARM64 CPU and Interrupt Monitor:\n\n");

    cpu_write_vendor(vendor);
    printf("CPU Vendor: %s\n", vendor);
    printf("Hypervisor present: %s\n", cpu_hv() ? "Yes" : "No");

    // Get CPU frequency
    uint64_t cntfrq;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    double cntfrq_mhz = (double)cntfrq / 1000000;

    printf("CPU Frequency: %.2f MHz\n", cntfrq_mhz);
    printf("\nMonitoring interrupts. Press Ctrl+C to stop.\n\n");

    read_interrupts(interrupts_prev, &count_prev);
    uint64_t last_check_time = get_system_time();

    int iteration = 0;
    while (running) {
        usleep(100000);  // Sleep for 100ms

        // Generate interrupts every 10 iterations (approximately every 1 second)
        if (++iteration % 10 == 0) {
            generate_interrupts();
        }

        uint64_t current_time = get_system_time();
        read_interrupts(interrupts_curr, &count_curr);

        double elapsed_ms = (double)(current_time - last_check_time) / cntfrq_mhz / 1000.0;

        for (int i = 0; i < count_curr; i++) {
            int prev_index = -1;
            for (int j = 0; j < count_prev; j++) {
                if (interrupts_curr[i].irq == interrupts_prev[j].irq) {
                    prev_index = j;
                    break;
                }
            }

            if (prev_index != -1) {
                unsigned long long count_diff = interrupts_curr[i].count - interrupts_prev[prev_index].count;
                if (count_diff > 0) {
                    double avg_time_between_ms = elapsed_ms / count_diff;

                    printf("Interrupt: IRQ %d, Name: %-20s, Count: %-5llu, Elapsed Time: %.3f ms, Avg Time Between: %.3f ms\n",
                           interrupts_curr[i].irq, interrupts_curr[i].name, count_diff, elapsed_ms, avg_time_between_ms);
                }
            }
        }

        memcpy(interrupts_prev, interrupts_curr, sizeof(InterruptInfo) * count_curr);
        count_prev = count_curr;
        last_check_time = current_time;
    }

    printf("\nInterrupt monitoring stopped.\n");

    return 0;
}
