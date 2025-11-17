#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/select.h>
#include "primer-server.h"

volatile sig_atomic_t g_end = 0;
pthread_mutex_t g_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
uint8_t g_clients = 0;
int g_sockfd1;
int g_sockfd2;

void *find_largest_prime(void *arg){
	int socket = *(int *)arg;
	free(arg);

	pthread_mutex_lock(&g_clients_mutex);
	g_clients++;
	pthread_mutex_unlock(&g_clients_mutex);

	// run until client terminates
	while(1){
		uint32_t count;
		// Each value will be guaranteed to be 4 bytes
		ssize_t len = recv(socket, &count, sizeof(uint32_t), MSG_WAITALL);
		if(len < 1){
			break;
		}

		count = ntohl(count);
		LOG("Client sent count of %" PRIu32 "\n", count);
		// If user sends count of 0, they are done
		if(count == 0){
			break;
		}

		uint32_t num_array[MAX_NUMS];
		len = recv(socket, num_array, count * sizeof(uint32_t), MSG_WAITALL);
		if(len < 1){
			break;
		}
		LOG("Client sent array containing: ");

		for(uint32_t i = 0 ; i < count ; i++){
			num_array[i] = ntohl(num_array[i]);
			LOG("%"PRIu32" ", num_array[i]);
			for(uint32_t j = num_array[i] ; j > 1 ; j--){
				if(is_prime(j)){
					num_array[i] = j;
					break;
				}
						
			}
			LOG("-> %" PRIu32 ", ", num_array[i]);
			num_array[i] = htonl(num_array[i]);
		}
		LOG("\n");

		send(socket, num_array, count * sizeof(uint32_t), 0);
		LOG("Sent primes to client\n");
	}

	/* Cleanup */
	close(socket);
	pthread_mutex_lock(&g_clients_mutex);
	g_clients--;
	pthread_mutex_unlock(&g_clients_mutex);
	LOG("Closed client socket and thread\n");

	return NULL;
}

bool is_prime(uint32_t num){
	if(num < 2 || num % 2 == 0){
		return false;
	} else if(num == 2){
		return true;
	}

	for(uint32_t i = 2 ; i < num ; i++){
		if(num % i == 0){
			return false;
		}
	}

	return true;
}

int main(int argc, char *argv[]) {
	/* Commandline stuff */
	if(argc < 2){
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	uint16_t port = 0;
	int opt;

	while((opt = getopt(argc, argv, "p:h")) != -1){
		switch(opt){
			case 'p':
				int64_t temp_port = strtoll(optarg, NULL, 10);
				if(temp_port < 1 || temp_port > UINT16_MAX){
					fprintf(stderr, "error: invalid port: integer must be greater than 0 and less than or equal to %" PRIu16 "\n"
							"usage: %s -p <positive integer>\n", UINT16_MAX, argv[0]);
					exit(EXIT_FAILURE);
				}
				port = (uint16_t)temp_port;
				break;
			case 'h':
			default:
				print_usage(argv[0]);
				return EXIT_SUCCESS;
		}
	}

	/* Variable setup */
	struct sockaddr_in server;
	signal(SIGINT, handle_sigint);

	g_sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
	if(g_sockfd1 < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* build the socket info */
	int option = 1;
	setsockopt(g_sockfd1, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	/* Bind and Listen */
	if(bind(g_sockfd1, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if(listen(g_sockfd1, 64) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

	fd_set inSet;
	struct sockaddr_in client;
	socklen_t clen = sizeof(client);

	printf("Server is listening on port %d. Ctrl + C to stop.\n", ntohs(server.sin_port));

	FD_ZERO(&inSet);
	FD_SET(g_sockfd1, &inSet);
	FD_SET(STDIN_FILENO, &inSet);
	/* Accept */
	while(!g_end){
		// Loop until SIGINT or STDIN
		if(select(g_sockfd1 + 1, &inSet, NULL, NULL, NULL) < 0){
			perror("select");
			break;
		}

		// Client received
		if(FD_ISSET(g_sockfd1, &inSet)){
			g_sockfd2 = accept(g_sockfd1, (struct sockaddr *) &client, &clen);
			if(g_sockfd2 < 0 && !g_end){
				perror("accept");
				continue;
			}
			if(!g_end){
				LOG("Client accepted\n");
			}

			int *s = (int *)malloc(sizeof(int));
			pthread_t tid;

			*s = g_sockfd2;
			// one thread per client
			int error = pthread_create(&tid, NULL, find_largest_prime, (void *) s);
			if(error){
				fprintf(stderr, "pthread_create error: %d\n", error);
				close(g_sockfd2);
				free(s);
				continue;
			}

			pthread_detach(tid);
		}

		// Stop received
		if(FD_ISSET(STDIN_FILENO, &inSet)){
			handle_sigint(SIGINT);
			break;
		}

		FD_ZERO(&inSet);
		FD_SET(g_sockfd1, &inSet);
		FD_SET(STDIN_FILENO, &inSet);
	}

	/* Wait for threads */
	while(1){
		pthread_mutex_lock(&g_clients_mutex);
		uint8_t count = g_clients;
		pthread_mutex_unlock(&g_clients_mutex);
		if(count == 0){
			break;
		}
		usleep(100000);
	}

	close(g_sockfd1);
	return EXIT_SUCCESS;
}

void handle_sigint(int sig){
	(void)sig;
	g_end = 1;
	close(g_sockfd1);
	LOG("\nInterrupt received, shutting down...\n");
}

void print_usage(char *prog_name){
	printf("usage: ./%s <-p> <port> [options]\n"
			"\t-p <port>\tport number the server should listen on\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name);
}
