#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#define PRINT_LOGS false 
#define MAX_NUMS 20
#define LOG(log, ...) \
	do { if(PRINT_LOGS) printf(log, ##__VA_ARGS__); } while(0)

void print_usage(char *prog_name){
	printf("usage: ./%s <-a> <address> <-p> <port> <-f> <filepath> [options]\n"
			"\t-a <address>\t the address the client should connect to\n"
			"\t-p <port>\tport number the client should connect to\n"
			"\t-f <filepath>\tthe file the client will read from\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name);
}

int main(int argc, char *argv[]) {
	if(argc < 7){
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int opt;
	uint16_t port = 0;
	char *address = NULL;
	char *filename = NULL;

	while((opt = getopt(argc, argv, "a:p:f:h")) != -1){
		switch(opt){
			case 'a':
				address = strdup(optarg);
				break;
			case 'f':
				filename = strdup(optarg);
				break;
			case 'p':
				int64_t temp_port = strtoll(optarg, NULL, 10);
				if(temp_port < 1 || temp_port > 65535){
					fprintf(stderr, "error: invalid port: integer must be greater than 0 and less than 65536\n"
							"usage: %s -p <int>\n", argv[0]);
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

	FILE *fp = fopen(filename, "r");
	if(!fp){
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	int sockfd;
	struct sockaddr_in server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_port = htons(port);

	if(connect(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0){
		perror("connect");
		exit(EXIT_FAILURE);
	}
	printf("Successfully connected to %s:%" PRIu16 "\n", address, port);

	char buf[256];
	uint32_t num_array[MAX_NUMS];
	// Loop until EOF
	while(fgets(buf, sizeof(buf), fp)){
		char *token = strtok(buf, " \t\n\t\r");
		uint32_t count = 0;

		while(token && count < MAX_NUMS){
			char *end;
			uint64_t num = strtoull(token, &end, 10);
			LOG("read token %s converted to %" PRIu64" \n", token, num);
			if(*end != '\0' || num < 1 || num > UINT32_MAX){
				fprintf(stderr, "error: invalid integer detected: %s\n", token);
			} else{
				num_array[count] = htonl((uint32_t)num);
				count++;
			}
			token = strtok(NULL, " \t\n\r");
		}

		if(count == 0){
			continue;
		}

		uint32_t ncount = htonl(count);
		if(send(sockfd, &ncount, sizeof(ncount), 0) < 0){
			perror("count");
			break;
		}
		LOG("Sent count %" PRIu32 " to server\n", count);

		if(send(sockfd, num_array, count * sizeof(uint32_t), 0) < 0){
			perror("array");
			break;
		}
		LOG("Sent ");
		for(uint32_t i = 0 ; i < count ; i++){
			LOG("%" PRIu32 " ", ntohl(num_array[i]));
		}
		LOG("to server\n");

		ssize_t len = recv(sockfd, num_array, count * sizeof(uint32_t), MSG_WAITALL); 
		if(len < 1){
			perror("recv");
			break;
		}

		printf("Response from server: ");
		for(uint32_t i = 0 ; i < count ; i++){
			num_array[i] = ntohl(num_array[i]);
			printf("%" PRIu32 " ", num_array[i]);
		}
		printf("\n");
	}

	uint32_t stop = 0;
	stop = htonl(stop);
	send(sockfd, &stop, sizeof(uint32_t), 0);

	free(filename);
	free(address);
	fclose(fp);
	close(sockfd);
	printf("Disconnected from server\n");
	return EXIT_SUCCESS;
}
