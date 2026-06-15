#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define DISK_MAGIC "TPSO2FS"
#define DISK_MAGIC_SIZE 8

#define MIN_DISK_SIZE (64 * 1024)
#define MAX_DISK_SIZE (64 * 1024 * 1024)

#define MIN_BLOCK_SIZE 64
#define MAX_BLOCK_SIZE 4096

#define DEFAULT_DISK_PATH "disco_simulado.bin"

typedef enum {
    OPERATION_OK = 0,
    OPERATION_ERROR = 1
} OperationStatus;

typedef enum {
    BLOCK_FREE = 0,
    BLOCK_USED = 1
} BlockStatus;

#endif
