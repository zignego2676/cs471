/*
 * Benjamin Zignego 
 * 12/05/2025
 */

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define MESSAGE_LENGTH 100
#define PRINT_LOGS true

#define LOG(log, ...) \
	do { if(PRINT_LOGS) printf(log, ##__VA_ARGS__); } while(0)

#endif
