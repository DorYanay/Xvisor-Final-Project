#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define NUM_SAMPLES 1000

static inline uint64_t get_system_time() {
    uint64_t time;
    asm volatile("mrs %0, cntvct_el0" : "=r" (time));
    return time;
}

static inline uint64_t time_diff() {
    uint64_t start, end;
    start = get_system_time();
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
        // No sleep between samples to capture fine-grained differences
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

void print_timing_stats(uint64_t *results, int num_samples) {
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

    printf("Timing Statistics (in cycles):\n");
    printf("  Samples: %d\n", num_samples);
    printf("  Non-zero samples: %d\n", non_zero_count);
    if (non_zero_count > 0) {
        printf("  Minimum: %llu\n", min);
        printf("  Maximum: %llu\n", max);
        printf("  Average: %.2f\n", (double)sum / non_zero_count);
    } else {
        printf("  All samples were zero\n");
    }
}

int main() {
    char vendor[13] = {0};
    uint64_t timing_results[NUM_SAMPLES];

    printf("ARM64 CPU Detection Results:\n");

    cpu_write_vendor(vendor);
    printf("CPU Vendor: %s\n", vendor);

    printf("Hypervisor present: %s\n", cpu_hv() ? "Yes" : "No");

    printf("Running timing test...\n");
    cpu_timing_test(timing_results, NUM_SAMPLES);
    print_timing_stats(timing_results, NUM_SAMPLES);

    return 0;
}
