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
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include "project.h"

#define PROBABILITY_MAX 1000

static void print_usage(char *prog_name){
	printf("usage: ./%s <-r> <port> <-f> <address> <-s> <port> <-t> <address> <-d> <port> [options]\n"
			"\t-r <port>\tthe port of the router\n"
			"\t-f <address>\tthe address of the sender\n"
			"\t-s <port>\tthe port of the sender\n"
			"\t-t <address>\tthe address of the receiver\n"
			"\t-d <port>\tthe port of the receiver\n"
			"\n\t\taddress\tan IPv4 address\n\t\tport: a positive integer < 65536\n"
			"\t-p <num>\tthe probability that a packet will be dropped\n\t\tnum: an integer between 0 and 1000\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name
	);
}

int main(int argc, char *argv[]){	
	int opt;

	uint16_t router_port = 0;
	char *sender_address = NULL;
	uint16_t sender_port = 0;
	char *receiver_address = NULL;
	uint16_t receiver_port = 0;
	uint16_t probability = 0;
	while((opt = getopt(argc, argv, "r:f:s:t:d:p:h")) != -1){
		int64_t temp_num;
		switch(opt){
			case 'r':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				router_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'f':
				sender_address = strdup(optarg);
				break;
			case 's':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				sender_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 't':
				receiver_address = strdup(optarg);
				break;
			case 'd':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				receiver_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'p':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > PROBABILITY_MAX || temp_num < 0){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				probability = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'h':
			case '?':
			default:
				print_usage(argv[0]);
				return EXIT_SUCCESS;
		}
	}

	srand(time(NULL));

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in router;
	memset(&router, 0, sizeof(router));
	router.sin_family = AF_INET;
	router.sin_addr.s_addr = INADDR_ANY;
	router.sin_port = htons(router_port);
	if(bind(sockfd, (struct sockaddr *)&router, sizeof(router)) < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in sender;
	memset(&sender, 0, sizeof(sender));
	sender.sin_family = AF_INET;
	sender.sin_addr.s_addr = inet_addr(sender_address);
	sender.sin_port = htons(sender_port);

	struct sockaddr_in receiver;
	memset(&receiver, 0, sizeof(receiver));
	receiver.sin_family = AF_INET;
	receiver.sin_addr.s_addr = inet_addr(receiver_address);
	receiver.sin_port = htons(receiver_port);

	printf("Router successfully started on port %u with sender %s:%u and receiver %s:%u\n", router_port, sender_address, sender_port, receiver_address, receiver_port);
	printf("Packet drop probability of %0.2f%%\n", probability / (double)10); 

	fd_set inSet;

	while(true){
		FD_ZERO(&inSet);
		FD_SET(sockfd, &inSet);
		FD_SET(STDIN_FILENO, &inSet);
		if(select(sockfd + 1, &inSet, NULL, NULL, NULL) < 0){
			perror("select");
			break;
		}

		if(FD_ISSET(STDIN_FILENO, &inSet)){
			printf("Shutdown received\n");
			break;
		}

		if(FD_ISSET(sockfd, &inSet)){
			struct sockaddr_in from;
			socklen_t flen = sizeof(from);
			char buf[MESSAGE_LENGTH + HEADER_SIZE];
			ssize_t ret = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &flen);

			if(ret < 0){
				perror("recvfrom");
				continue;
			}

			struct sockaddr_in *dest;

			if(from.sin_port == htons(sender_port) && from.sin_addr.s_addr == inet_addr(sender_address)){			// Sender
				dest = &receiver;
			} else if(from.sin_port == htons(receiver_port) && from.sin_addr.s_addr == inet_addr(receiver_address)){	// Receiver
				dest = &sender;
			} else{														// Error
				printf("Ignoring unexpected packet received\n");
				continue;
			}

			if((rand() % PROBABILITY_MAX) < probability){
				if(sendto(sockfd, buf, ret, 0, (struct sockaddr *)dest, sizeof(*dest)) < 0){
					perror("sendto");
				}
			} else{
				printf("Packet dropped\n");
				continue;
			}
		}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}
