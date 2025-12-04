/*
 * Benjamin Zignego
 * 12/5/2025
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "project.h"

static void print_usage(char *prog_name){
	printf("usage: ./%s <-m> <address> <-r> <port> <-d> <port> -o <filepath> [options]\n"
			"\t-m <address>\tthe address of the router\n"
			"\t-r <port>\tthe port of the router\n"
			"\t-d <port>\tthe port of the receiver\n"
			"\t-o <filepath\tthe name of the output file\n"
			"\t\tfilepath\ta valid UNIX filepath\n"
			"\t\taddress\tan IPv4 address\n"
			"\t\tport: a positive integer <= %" PRIu16 "\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name, UINT16_MAX
	);
}


int main(int argc, char *argv[]){
	int opt;

	uint16_t receiver_port = 0;
	char *router_address = NULL;
	uint16_t router_port = 0;
	char *output_file = NULL;

	if(argc != 9){
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	while((opt = getopt(argc, argv, "m:r:d:o:h")) != -1){
		int64_t temp_num;
		switch(opt){
			case 'm':
				router_address = strdup(optarg);
				break;
			case 'r':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				router_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'd':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				receiver_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'o':
				output_file = strdup(optarg);
				break;
			case 'h':
			case '?':
			default:
				print_usage(argv[0]);
				return EXIT_SUCCESS;
		}
	}

	FILE *output = fopen(output_file, "wb");
	if(!output){
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in receiver;
	memset(&receiver, 0, sizeof(receiver));
	receiver.sin_family = AF_INET;
	receiver.sin_addr.s_addr = INADDR_ANY;
	receiver.sin_port = htons(receiver_port);
	if(bind(sockfd, (struct sockaddr *)&receiver, sizeof(receiver)) < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in router;
	memset(&router, 0, sizeof(router));
	router.sin_family = AF_INET;
	router.sin_addr.s_addr = inet_addr(router_address);
	router.sin_port = htons(router_port);

	printf("Receiver ready on port %" PRIu16 " with router %s:%" PRIu16 "\n", receiver_port, router_address, router_port);

	uint8_t expected_seq = 0;

	while(true){
		size_t len = MESSAGE_LENGTH + HEADER_SIZE + 1; // Add 1 for the null terminator
		char buf[len];
		struct sockaddr_in from;
		socklen_t flen = sizeof(from);

		ssize_t ret = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &flen);
		buf[len] = '\0';
		if(ret < 0){
			perror("recvfrom");
			continue;
		}

		if(ret < HEADER_SIZE){
			continue;
		}

		uint8_t received_flags = buf[0];
		uint8_t received_seq = buf[1];
		LOG("Received sequence %" PRIu8 " and flags %x\n", received_seq, received_flags);

		if((received_flags & 0x0f) != DATA){
			continue;
		}

		char ack[HEADER_SIZE];
		if(received_seq != expected_seq){
			ack[0] = ACK;
			ack[1] = expected_seq ^ 1;

			printf("Duplicate packet received of sequence %" PRIu8 " instead of expected %" PRIu8 "\n", received_seq, expected_seq);
			
			sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&router, sizeof(router));
			continue;
		}

		printf("Received %s\n", buf + HEADER_SIZE);
		fwrite(buf + HEADER_SIZE, 1, (uint8_t)buf[2], output);
		printf("Wrote data to file\n");

		ack[0] = ACK;
		ack[1] = received_seq;
		sendto(sockfd, ack, HEADER_SIZE, 0, (struct sockaddr *)&router, sizeof(router));

		expected_seq ^= 1;

		if((received_flags & 0xf0) == IS_EOF){
			LOG("EOF received\n");
			break;
		} 
	}

	printf("Receiver shutting down...\n");

	fclose(output);
	close(sockfd);
	free(router_address);

	return EXIT_SUCCESS;
}
