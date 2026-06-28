#ifndef INTERFACE_UTILS_H
#define INTERFACE_UTILS_H

#include <stddef.h>

#include "disco/disco.h"
#include "i-node/inode.h"

void trimLine(char *text);
void removeOuterQuotes(char *text);

int initRoot(VirtualDisk *disk, Inode inodeTable[]);
int resolvePath(VirtualDisk *disk, Inode inodeTable[], const char *path);
int splitParentAndName(const char *path,
                       char *parentPath,
                       size_t parentSize,
                       char *name,
                       size_t nameSize);
int resolveParent(VirtualDisk *disk,
                  Inode inodeTable[],
                  const char *path,
                  char *name,
                  size_t nameSize);

#endif