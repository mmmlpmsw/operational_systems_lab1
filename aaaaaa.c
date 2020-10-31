#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <cmath.h>

#define MALLOC_SIZE 84000000
#define FILL_THREADS 127
#define BLOCK_SIZE 33
#define FILE_SIZE 185000000
#define FILE_COUNT MALLOC_SIZE / FILE_SIZE + (MALLOC_SIZE % FILE_SIZE == 0 ? 0 : 1)

struct ThreadsArgs {
    int fd;
    void* address;
    size_t size;
};

// для рандомной записи блоков в файл (отслеживать свободные промежут очки блоков)
struct LinkedListNode {
    int startBlock;
    size_t size;
    struct LinkedListNode* next;
};

void* fillMemory();
void fillMemoryRegion(int, void*, size_t);
void* fillMemoryRegionProxy(void*);
void writeRegionToFile(void*);
void writeFile(int, void*, size_t);

int main() {
    while (1) {
        void* regionPointer = fillMemory();
        writeRegionToFile(regionPointer);

        free(regionPointer);
    }
}

void* fillMemory() {
    void * regionPointer = malloc(MALLOC_SIZE);

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        printf("Cannot open /dev/urandom\n");
        return regionPointer;
    }
    pthread_t threads[FILL_THREADS];
    struct ThreadsArgs threadArgs[FILL_THREADS];

    size_t size = MALLOC_SIZE/FILL_THREADS;
    for (int i = 0; i < FILL_THREADS; i++) {
        threadArgs[i].fd = fd;
        if (i == FILL_THREADS - 1)
            threadArgs[i].size = size + MALLOC_SIZE % FILL_THREADS;
        else
            threadArgs[i].size = size;
        threadArgs[i].address = (void*)((uint8_t *)regionPointer + i * size);
        pthread_create(&threads[i], 0, fillMemoryRegionProxy, &threadArgs[i]);
    }

    for (int i = 0; i < FILL_THREADS; i++)
        pthread_join(threads[i], NULL);

    close(fd);
    return regionPointer;
}

void writeRegionToFile(void* regionPtr) {
    for (int i = 0; i < FILE_COUNT; i++) {
        char fileName[i + 2];
        for (int c = 0; c < i + 1; c++)
            fileName[c] = 'a';
        fileName[i + 1] = '\0';
        int fd = open(fileName, O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd < 0) {
            printf("Cannot create file\n");
            return;
        }
        if (i == FILE_COUNT - 1)
            writeFile(fd, (void*)((uint8_t *)regionPtr + i * FILE_SIZE), MALLOC_SIZE - FILE_SIZE * (FILE_COUNT - 1));
        else writeFile(fd, (void*)((uint8_t *)regionPtr + i * FILE_SIZE), FILE_SIZE);
    }
}

void writeFile(int fd, void* address, size_t size) {
    int count = size / BLOCK_SIZE + (size % BLOCK_SIZE == 0 ? 0 : 1);
    struct LinkedListNode node;
    node.size = count;
    node.startBlock = 0;
    node.next = NULL;

    while(1) {
        int i = pickRandomBlock(node, count);
        if (i == -1)
            break;
        lseek(fd, i * BLOCK_SIZE, SEEK_SET);
        if (i == count - 1)
            write(fd, (void*)((uint8_t *)address + i * count), size - BLOCK_SIZE * (count - 1));
        else write(fd, (void*)((uint8_t *)address + i * count), BLOCK_SIZE);
        printf("Writing %d blocks of %d\r", i, count);
        fflush(stdout);
    }
}
/**
 * Выбирает случайный блок с помощью списка промежутков
 * и возвращает номер этого блока
 * Этот блок удаляется из списка промежутков и, если свободных блоков не осталось, он возвращает -1
*/
int pickRandomBlock(struct LinkedListNode * node, int count) {
    int randomNumber = rand() % (count + 1);
//    if (randomNumber)

    node->size += 1;


}

void* fillMemoryRegionProxy(void* argsPointer) {
    struct ThreadsArgs* args = argsPointer;
    fillMemoryRegion(args->fd, args->address, args->size);
    return NULL;
}

void fillMemoryRegion(int fd, void* address, size_t size) {
    read(fd, address, size);
}