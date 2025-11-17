#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

/* Defines */
#define MAX_NUMS 21
#define PRINT_LOGS true

#define LOG(log, ...) \
	do { if(PRINT_LOGS) printf(log, ##__VA_ARGS__); } while(0)

/* Globals */
volatile sig_atomic_t g_end; 
extern int g_sockfd1, g_sockfd2;
pthread_mutex_t g_numClients_mutex;
uint8_t g_clients;

/* Functions */
void print_usage(char *prog_name);
void handle_sigint(int sig);
void *find_largest_prime(void *arg);
bool is_prime(uint32_t num);
