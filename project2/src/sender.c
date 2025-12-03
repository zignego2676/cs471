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
	printf("usage: ./%s <-m> <address> <-r> <port> <-s> <port> <-i> <filepath> <-t> <num> [options]\n"
			"\t-m <address>\tthe address of the router\n"
			"\t-r <port>\tthe port of the router\n"
			"\t-s <port>\tthe port of the sender\n"
			"\n\t\taddress\tan IPv4 address\n"
			"\t\tport: a positive integer <= %" PRIu16 "\n"
			"\t-i <filepath>\tthe name of the input file \n"
			"\t\tfilepath\ta valid UNIX filepath\n"
			"\t-t <num>\tthe timeout value in milliseconds\n"
			"\t\tnum\ta positive integer\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name, UINT16_MAX
	);
}

int main(int argc, char *argv[]){	
	if(argc != 6){
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int opt;

	char *router_address = NULL;
	uint16_t router_port = 0;
	uint16_t sender_port = 0;
	char *input_file = NULL;
	uint64_t timeout_ms = 0;

	while((opt = getopt(argc, argv, "m:r:s:i:t:h")) != -1){
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
			case 's':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				sender_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'i':
				input_file = strdup(optarg);
				break;
			case 't':
				temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num < 0){
					print_usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				timeout_ms = (uint64_t)temp_num;
				break;
			case 'h':
			case '?':
			default:
				print_usage(argv[0]);
				return EXIT_SUCCESS;
		}
	}

	FILE *fp = fopen(input_file, "rb");
	if(!fp){
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(struct sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(sender_port);
	if(bind(sockfd, (struct sockaddr *)&local, sizeof(struct sockaddr_in)) < 0){
		perror("bind");
		close(sockfd);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in router;
	memset(&router, 0, sizeof(struct sockaddr_in));
	router.sin_family = AF_INET;
	router.sin_addr.s_addr = inet_addr(router_address);
	router.sin_port = htons(router_port);

	struct timeval x;
	x.tv_sec = timeout_ms / 1000;
	x.tv_usec = (timeout_ms % 1000) * 1000;

	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &x, sizeof(struct timeval)) < 0){
		perror("setsockopt");
		close(sockfd);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in from;
	socklen_t flen = sizeof(from);

	uint8_t seq = 0;
	uint64_t chunk = 0;
	ssize_t ret;
	size_t len = MESSAGE_LENGTH + HEADER_SIZE;

	char buf[len];

	while(true){
		size_t read_bytes = fread(buf + HEADER_SIZE, sizeof(char), MESSAGE_LENGTH, fp);
		LOG("Read %s from file\n", buf + HEADER_SIZE);
		int8_t eof = NOT_EOF;
		if(read_bytes == 0){
			break;
		} else if(read_bytes < MESSAGE_LENGTH){
			eof = IS_EOF;
		}

		buf[0] = DATA | eof;
		buf[1] = seq;
		bool acked = false;
		while(!acked){
			ret = sendto(sockfd, buf, len, 0, (struct sockaddr *)&router, sizeof(router)); 
			LOG("Packet of sequence %" PRIu8 " and chunk %" PRIu64 "was sent\n", seq, chunk);
			if(ret < 0){
				perror("sendto");
				break;
			}

			char ack_buf[HEADER_SIZE];
			ret = recvfrom(sockfd, ack_buf, sizeof(ack_buf), 0, (struct sockaddr *)&from, &flen);
			if(ret >=0){
				if(ret >= 2 && (ack_buf[0] & 0x0f) == ACK && ack_buf[1] == seq){
					printf("Chunk %" PRIu64 " of sequence %" PRIu8 "acked\n", chunk, seq);
					acked = true;
					seq ^= 1;
				} else{
					continue;
				}
			} else{
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					fprintf(stderr, "Timed-out waiting for an ACK for chunk %" PRIu64 "\n", chunk);
					continue;
				} else if(errno == EINTR){
					continue;
				} else{
					perror("recvfrom");
					break;
				}
			}
		}

		chunk++;

		if(eof){
			LOG("Last chunk sent and acknowledged\n");
			break;
		}
	}

	printf("Sender shutting down...\n");
	fclose(fp);
	close(sockfd);
	return EXIT_SUCCESS;
}
