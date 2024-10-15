#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

#define TRUE 1
#define FALSE 0
#define NUM_SAMPLES 1000
#define NUM_FORK_TESTS 10
#define TIMING_ITERATIONS 1000

static inline uint64_t get_system_time() {
    uint64_t time;
    asm volatile("mrs %0, cntvct_el0" : "=r" (time));
    return time;
}

static inline uint64_t time_diff() {
    uint64_t start, end;
    start = get_system_time();
    for (volatile int i = 0; i < TIMING_ITERATIONS; i++) {
        // This loop is to ensure we measure a non-zero time difference
    }
    end = get_system_time();
    return end - start;
}

static inline void get_cpu_info(char* vendor) {
    uint64_t midr;
    asm volatile("mrs %0, midr_el1" : "=r" (midr));
    
    uint8_t implementer = (midr >> 24) & 0xFF;
    uint8_t variant = (midr >> 20) & 0xF;
    uint8_t architecture = (midr >> 16) & 0xF;
    uint16_t part_num = (midr >> 4) & 0xFFF;
    
    sprintf(vendor, "ARM%02X%01X%01X%03X", implementer, variant, architecture, part_num);
}

void cpu_timing_test(uint64_t *results, int num_samples) {
    for (int i = 0; i < num_samples; i++) {
        results[i] = time_diff();
    }
}

int cpu_hv() {
    uint64_t id_aa64pfr0;
    asm volatile("mrs %0, id_aa64pfr0_el1" : "=r" (id_aa64pfr0));
    return ((id_aa64pfr0 >> 40) & 0xF) != 0 ? TRUE : FALSE;
}

void cpu_write_vendor(char* vendor) {
    get_cpu_info(vendor);
}

uint64_t measure_fork_time() {
    uint64_t start, end;
    pid_t pid;

    start = get_system_time();
    pid = fork();
    
    if (pid == 0) {
        // Child process
        _exit(0);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
        end = get_system_time();
        return end - start;
    } else {
        // Fork failed
        perror("fork");
        return 0;
    }
}

void print_timing_stats(uint64_t *results, int num_samples, double cntfrq_mhz) {
    uint64_t min = UINT64_MAX, max = 0, sum = 0;
    int non_zero_count = 0;

    for (int i = 0; i < num_samples; i++) {
        if (results[i] > 0) {
            if (results[i] < min) min = results[i];
            if (results[i] > max) max = results[i];
            sum += results[i];
            non_zero_count++;
        }
    }

    printf("Timing Statistics:\n");
    printf("  Samples: %d\n", num_samples);
    printf("  Non-zero samples: %d\n", non_zero_count);
    if (non_zero_count > 0) {
        printf("  Minimum: %.6f ms\n", min / (cntfrq_mhz * 1000));
        printf("  Maximum: %.6f ms\n", max / (cntfrq_mhz * 1000));
        printf("  Average: %.6f ms\n", (sum / non_zero_count) / (cntfrq_mhz * 1000));
        printf("  Average per iteration: %.9f ms\n", ((sum / non_zero_count) / TIMING_ITERATIONS) / (cntfrq_mhz * 1000));
    } else {
        printf("  All samples were zero\n");
    }
}

int main() {
    char vendor[13] = {0};
    uint64_t timing_results[NUM_SAMPLES];
    uint64_t cntfrq;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    double cntfrq_mhz = (double)cntfrq / 1000000;

    printf("ARM64 CPU Detection Results:\n");
    cpu_write_vendor(vendor);
    printf("CPU Vendor: %s\n", vendor);
    printf("Hypervisor present: %s\n", cpu_hv() ? "Yes" : "No");
    printf("CPU Frequency: %.2f MHz\n", cntfrq_mhz);

    printf("\nRunning CPU timing test...\n");
    cpu_timing_test(timing_results, NUM_SAMPLES);
    print_timing_stats(timing_results, NUM_SAMPLES, cntfrq_mhz);

    printf("\nRunning fork timing test...\n");
    uint64_t total_fork_time = 0;
    for (int i = 0; i < NUM_FORK_TESTS; i++) {
        uint64_t fork_time = measure_fork_time();
        total_fork_time += fork_time;
        printf("Fork test %d: %.6f ms\n", i + 1, fork_time / (cntfrq_mhz * 1000));
    }
    printf("Average fork time: %.6f ms\n", (total_fork_time / NUM_FORK_TESTS) / (cntfrq_mhz * 1000));

    return 0;
}
