/*
 * Benjamin Zignego
 * 12/5/2025
 */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

static void print_usage(){
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
		switch(opt){
			case 'r':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage();
					exit(EXIT_FAILURE);
				}
				router_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'f':
				sender_address = strdup(optarg);
				break;
			case 's':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage();
					exit(EXIT_FAILURE);
				}
				sender_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 't':
				receiver_address = strdup(optarg);
				break;
			case 'd':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage();
					exit(EXIT_FAILURE);
				}
				receiver_port = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'p':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > 1000 || temp_num < 0){
					print_usage();
					exit(EXIT_FAILURE);
				}
				probability = (uint16_t)strtol(optarg, NULL, 10);
				break;
			case 'h':
			default:
				print_usage();
				return EXIT_SUCCESS;
		}
	}


	return EXIT_SUCCESS;
}
