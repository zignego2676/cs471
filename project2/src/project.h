/*
 * Benjamin Zignego 
 * 12/05/2025
 */

#ifndef _PROJECT_H_
#define _PROJECT_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MESSAGE_LENGTH 100
#define HEADER_SIZE 2
#define PRINT_LOGS true
#define DATA 0x01
#define ACK 0x02
#define NOT_EOF 0x10
#define IS_EOF 0x20

#define LOG(log, ...) \
	do { if(PRINT_LOGS) printf(log, ##__VA_ARGS__); } while(0)

#endif
