#ifndef INTERFACE_VIEW_H
#define INTERFACE_VIEW_H

#include "disco/disco.h"

#define C_RESET   "\033[0m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_CYAN    "\033[36m"
#define C_BOLD    "\033[1m"

void printErrorMsg(const char *message);
void printInterfaceHelp(void);
void printBanner(const char *diskPath, int interactive, int verbose);
void printPrettyDiskInfo(VirtualDisk *disk);

#endif