#include "interface_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "diretorio/diretorio.h"
#include "interface_view.h"

#define PATH_SIZE 512

void trimLine(char *text) {
    size_t len;

    if (text == NULL) {
        return;
    }

    while (isspace((unsigned char)*text)) {
        memmove(text, text + 1, strlen(text));
    }

    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

void removeOuterQuotes(char *text) {
    size_t len;

    if (text == NULL) {
        return;
    }

    trimLine(text);
    len = strlen(text);

    if (len >= 2 &&
        ((text[0] == '"' && text[len - 1] == '"') ||
         (text[0] == '\'' && text[len - 1] == '\''))) {
        memmove(text, text + 1, len - 2);
        text[len - 2] = '\0';
    }
}

int initRoot(VirtualDisk *disk, Inode inodeTable[]) {
    int root;

    if (disk == NULL || inodeTable == NULL) {
        return -1;
    }

    initializeInode(inodeTable);

    root = allocInode(inodeTable, DIRECTORY);
    if (root != 0) {
        return -1;
    }

    if (!addEntry(disk, inodeTable, root, ".", root, DIRECTORY)) {
        return -1;
    }

    if (!addEntry(disk, inodeTable, root, "..", root, DIRECTORY)) {
        return -1;
    }

    return root;
}

int resolvePath(VirtualDisk *disk, Inode inodeTable[], const char *path) {
    char copy[PATH_SIZE];
    char *token;
    int current = 0;

    if (path == NULL || strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
        return 0;
    }

    if (strlen(path) >= sizeof(copy)) {
        return -1;
    }

    strcpy(copy, path);
    token = strtok(copy, "/");

    while (token != NULL) {
        int next;

        if (inodeTable[current].type != DIRECTORY) {
            return -1;
        }

        next = searchEntry(disk, inodeTable, current, token);
        if (next == -1) {
            return -1;
        }

        current = next;
        token = strtok(NULL, "/");
    }

    return current;
}

int splitParentAndName(const char *path,
                       char *parentPath,
                       size_t parentSize,
                       char *name,
                       size_t nameSize) {
    const char *lastSlash;

    if (path == NULL ||
        parentPath == NULL ||
        name == NULL ||
        strlen(path) == 0 ||
        strcmp(path, "/") == 0) {
        return 0;
    }

    lastSlash = strrchr(path, '/');

    if (lastSlash == NULL) {
        snprintf(parentPath, parentSize, "/");
        snprintf(name, nameSize, "%s", path);
        return strlen(name) > 0;
    }

    if (*(lastSlash + 1) == '\0') {
        return 0;
    }

    snprintf(name, nameSize, "%s", lastSlash + 1);

    if (lastSlash == path) {
        snprintf(parentPath, parentSize, "/");
    } else {
        size_t len = (size_t)(lastSlash - path);

        if (len >= parentSize) {
            return 0;
        }

        memcpy(parentPath, path, len);
        parentPath[len] = '\0';
    }

    return strlen(name) > 0;
}

int resolveParent(VirtualDisk *disk,
                  Inode inodeTable[],
                  const char *path,
                  char *name,
                  size_t nameSize) {
    char parentPath[PATH_SIZE];
    int parent;

    if (!splitParentAndName(path, parentPath, sizeof(parentPath), name, nameSize)) {
        printErrorMsg("caminho invalido.");
        return -1;
    }

    parent = resolvePath(disk, inodeTable, parentPath);

    if (parent == -1 || inodeTable[parent].type != DIRECTORY) {
        printf("Erro: diretorio pai '%s' nao encontrado.\n", parentPath);
        return -1;
    }

    return parent;
}