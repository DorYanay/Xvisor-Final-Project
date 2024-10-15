#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define MAX_INTERRUPTS 256

typedef struct {
    int irq;
    char name[64];
    unsigned long long count;
} InterruptInfo;

volatile sig_atomic_t running = 1;

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

void clear_screen() {
    printf("\033[2J");
    printf("\033[H");
}

int main() {
    InterruptInfo interrupts_prev[MAX_INTERRUPTS], interrupts_curr[MAX_INTERRUPTS];
    int count_prev, count_curr;

    signal(SIGINT, signal_handler);

    printf("Interrupt Monitor\n\n");
    printf("Monitoring interrupts. Press Enter to refresh, 'r' to reset baseline, 'q' to quit.\n");
    printf("Perform actions in another terminal to generate interrupts.\n\n");

    read_interrupts(interrupts_prev, &count_prev);
    time_t last_check_time = time(NULL);

    char input[3];
    while (running) {
        if (fgets(input, sizeof(input), stdin)) {
            if (input[0] == 'r') {
                clear_screen();
                printf("Resetting interrupt baseline...\n");
                read_interrupts(interrupts_prev, &count_prev);
                last_check_time = time(NULL);
                continue;
            } else if (input[0] == 'q') {
                running = 0;
                break;
            }

            time_t current_time = time(NULL);
            read_interrupts(interrupts_curr, &count_curr);

            double elapsed_seconds = difftime(current_time, last_check_time);

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
                        double rate = count_diff / elapsed_seconds;

                        printf("Interrupt: IRQ %d, Name: %-20s, Count: %-5llu, Rate: %.2f/s\n",
                               interrupts_curr[i].irq, interrupts_curr[i].name, count_diff, rate);
                    }
                }
            }

            memcpy(interrupts_prev, interrupts_curr, sizeof(InterruptInfo) * count_curr);
            count_prev = count_curr;
            last_check_time = current_time;

            printf("\nPress Enter to refresh, 'r' to reset baseline, 'q' to quit.\n");
        }
    }

    printf("\nInterrupt monitoring stopped.\n");

    return 0;
}
