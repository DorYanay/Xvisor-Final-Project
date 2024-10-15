#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

#define TRUE 1
#define FALSE 0
#define NUM_FORK_TESTS 10

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

uint64_t cpu_timing_test() {
    int i;
    uint64_t avg = 0;
    for (i = 0; i < 10; i++) {
        avg += time_diff();
        usleep(500000);  // Sleep for 500ms
    }
    return avg / 10;
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

void cpu_write_brand(char* brand) {
    char vendor[13];
    get_cpu_info(vendor, brand);
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

int main() {
    char vendor[13] = {0};
    char brand[49] = {0};

    printf("ARM64 CPU Detection Results:\n");

    cpu_write_vendor(vendor);
    printf("CPU Vendor: %s\n", vendor);

    cpu_write_brand(brand);
    printf("CPU Brand: %s\n", brand);

    printf("Hypervisor present: %s\n", cpu_hv() ? "Yes" : "No");

    printf("Running timing test...\n");
    uint64_t timing_result = cpu_timing_test();
    printf("Average time difference: %llu cycles\n", timing_result);

    printf("\nMeasuring fork time...\n");
    uint64_t total_fork_time = 0;
    for (int i = 0; i < NUM_FORK_TESTS; i++) {
        uint64_t fork_time = measure_fork_time();
        total_fork_time += fork_time;
        printf("Fork test %d: %llu cycles\n", i + 1, fork_time);
    }
    printf("Average fork time: %llu cycles\n", total_fork_time / NUM_FORK_TESTS);

    return 0;
}
