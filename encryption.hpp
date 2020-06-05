#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <pthread.h>
#include <termios.h>
#include <vector>
#include <algorithm>
#include "structs.hpp"

#define DEFAULT_PASS "default"

int encrypt(Options* options, char* data, int size);
int decrypt(Options* options, char* data, int size);

/*
 * 	CUSTOMIZE THIS TWO METHODS
 *	
 *	Currently using basic XOR example encryption
 *	
 */

int encrypt(Options* options, char* data, int size)
{
	if(!options->encryption)
		return 0;
	
	for(int i = 0; i < size; i++)
		*(data + i) = *(data + i) ^ (int)options->password[0];

	return 0;
}

int decrypt(Options* options, char* data, int size)
{
	encrypt(options, data, size);

	return 0;
}
