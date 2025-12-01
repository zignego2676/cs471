/*
 * Benjamin Zignego
 * 12/5/2025
 */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

static void print_usage(){
	printf("usage: ./%s <-m> <address> <-r> <port> <-d> <port> -o <filepath> [options]\n"
			"\t-m <address>\tthe address of the router\n"
			"\t-r <port>\tthe port of the router\n"
			"\t-d <port>\tthe port of the receiver\n"
			"\t-o <filepath\tthe name of the output file\n"
			"\t\tfilepath\ta valid UNIX filepath\n"
			"\n\t\taddress\tan IPv4 address\n\t\tport: a positive integer < 65536\n"
			"options:\n"
			"\t-h\tdisplay this message and exit\n", prog_name
	);
}


int main(int argc, char *argv[]){
	int opt;

	uint16_t receiver_port = 0;
	char *router_address = NULL;
	uint16_t router_port = 0;
	char *output_file = NULL;

	while((opt = getopt(argc, argv, "m:r:d:o:h")) != -1){
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
			case 'd':
				int64_t temp_num = strtoll(optarg, NULL, 10); 
				if(temp_num > UINT16_MAX || temp_num < 1){
					print_usage();
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
				print_usage();
				return EXIT_SUCCESS;
		}
	}

	return EXIT_SUCCESS;
}
