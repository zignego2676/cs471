/*
 * Benjamin Zignego
 * 12/5/2025
 */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

static void print_usage(){
	printf("usage: ./%s <-m> <address> <-r> <port> <-s> <port> <-i> <filepath> <-t> <num> [options]\n"
			"\t-m <address>\tthe address of the router\n"
			"\t-r <port>\tthe port of the router\n"
			"\t-s <port>\tthe port of the sender\n"
			"\n\t\taddress\tan IPv4 address\n\t\tport: a positive integer < 65536\n"
			"\t-i <filepath>\tthe name of the input file \n"
			"\t\tfilepath\ta valid UNIX filepath\n"
			"\t-t <num>\tthe timeout value in milliseconds\n"
			"\t\tnum\ta positive integer\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name
	);
}

int main(int argc, char *argv[]){	
	int opt;

	char *router_address = NULL;
	uint16_t router_port = 0;
	uint16_t sender_port = 0;
	char *input_file = NULL;
	uint64_t timeout = 0;

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
				timeout = (uint64_t)temp_num;
				break;
			case 'h':
			default:
				print_usage();
				return EXIT_SUCCESS;
		}
	}


	return EXIT_SUCCESS;
}
