#ifndef INTERFACE_H
#define INTERFACE_H

int runInteractiveMode(const char *diskPath, int verboseInitial);
int runBatchMode(const char *diskPath, const char *commandsPath, int verboseInitial);

#endif
