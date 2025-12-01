/*
 * Benjamin Zignego
 * 12/5/2025
 */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

static void print_usage(char *prog_name){
	printf("usage: ./%s <-m> <address> <-r> <port> <-s> <port> <-i> <filepath> <-t> <num> [options]\n"
			"\t-m <address>\tthe address of the router\n"
			"\t-r <port>\tthe port of the router\n"
			"\t-s <port>\tthe port of the sender\n"
			"\n\t\taddress\tan IPv4 address\n\t\tport: a positive integer <= %" PRIu16 "\n"
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
		switch(opt){
			case 'm':
				router_address = strdup(optarg);
				break;
			case 'r':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage();
					exit(EXIT_FAILURE);
				}
				router_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 's':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage();
					exit(EXIT_FAILURE);
				}
				sender_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'i':
				input_file = strdup(optarg);
				break
			case 't':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num < 0){
					print_usage();
					exit(EXIT_FAILURE);
				}
				timeout_ms = (uint64_t)temp_num;
				break;
			case 'h':
			case '?':
			default:
				print_usage();
				return EXIT_SUCCESS;
		}
	}

	FILE *fp = fopen(input_file);
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
	uint64_t clen;
	ssize_t len = MESSAGE_LENGTH;
	char buf[MESSAGE_LENGTH];
	int ret;
	struct timeval x = malloc(sizeof(timeval));

	memset(&router, 0, sizeof(struct sockaddr_in));
	router.sin_family = AF_INET;
	router.sin_addr.s_addr = inet_addr(router_address);
	router.sin_port = htons(router_port);

	x.tv_sec = timeout_ms / 1000;
	x.tv_usec = timeout_ms * 1000;

	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &x, sizeof(struct timeval)) < 0){
		perror("setsockopt");
		close(sockfd);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	if(connect(sockfd, (struct sockaddr *) &router, sizeof(struct sockaddr_in)) < 0){
		perror("connect");
		exit(EXIT_FAILURE);
	}
	printf("Successfully connected to %s:%" PRIu16 "\n", router_address, router_port);

	while(fgets(buf, sizeof(buf), fp)){
		LOG("Read %s from file\n", buf);
		if(sendto(sockfd, buf, len, 0, (struct sockaddr *)&router, clen) < 0){
			perror("count");
			break;
		}

		ret = recvfrom(sockfd, buf, len, 0, NULL, NULL);
		while(ret == -1 && errno == EWOULDBLOCK){
			fprintf(stderr, "Timeout has occurred\n");
			sendto(sockfd, buf, len, 0, (struct sockaddr *)&router, clen);
			ret = recvfrom(sockfd, buf, len, 0, NULL, NULL);
		}
	}

	return EXIT_SUCCESS;
}
