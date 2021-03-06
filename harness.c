#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include "disk.h"
#include "filesystem.h"

typedef uint16_t uint16;

int n_tests = 0;

 // Run this many tests
void plan (int n) {
    printf("1..%d\n", n);
}

 // Simple boolean test
int ok (int t, const char* name) {
    n_tests++;
    if (!t) printf("not ");
    printf("ok %d - %s\n", n_tests, name);
    return t;
}

 // Print a comment
void diag (const char* message) {
    printf(" # %s\n", message);
}

 // Make sure it doesn't fail
void dRead2(int block, char* buf) {
    if (dRead(block, buf) == -1) {
        printf(" # Disk read error!\n");
        exit(1);
    }
}

int main () {
    plan(10);
    initDisk();
    ok(formatDisk() != -1, "formatDisk didn't return error");
     // Testing that all File structs were initialized to unallocated
    char buf [BLOCK_SIZE];
    dRead2(0, buf);
    int failed = 0;
    for (int i = 0; i < 4; i++) {
        if (buf[i * 0x20]) failed = 1;
    }
    ok(!failed, "formatDisk created four unallocated File entries");
     // Testing file opening, autocreates file
    int fh = fOpen("asdf.exe");
    ok(fh != -1, "fOpen didn't return error");
    dRead2(0, buf);
    ok(strncmp("asdf\0\0\0\0exe", buf, 11) == 0, "fOpen created a file record");
     // File closing and opening again
    ok(fClose(fh) != -1, "fClose didn't return error");
    fh = fOpen("asdf.exe");
    ok(fh != -1, "fOpen again didn't return error");
    dRead2(0, buf);
    ok(buf[0x20] == 0, "fOpen didn't create second file");
    uint16* first_cluster = (uint16*)(buf + 0x1a);
    ok(*first_cluster == 0, "first_cluster was set to 0 for new file");
     // Manually write a file.  TODO: get the File struct over here.
    *first_cluster = 2;
    dWrite(0, buf);
    for (int i = 0; i < BLOCK_SIZE-2; i++)
        buf[i] = 0xcc;
    buf[BLOCK_SIZE - 2] = 0;
    buf[BLOCK_SIZE - 1] = 0;
    dWrite(2, buf);
     // Testing file reading (according to what we just wrote)
    char buf2 [30];
    if (ok(fRead(fh, buf2, 30) != -1, "fRead didn't return error")) {
        int bad = 0;
        for (int i = 0; i < 30; i++)
            if (buf2[i] != 0xcc) bad = 1;
        ok(!bad, "fRead properly read file");
    }
    else {
        ok(0, "fRead properly read file # SKIP");
    }
     // Dump disk to file for perusing
    FILE* dump = fopen("disk.out", "w");
    if (!dump) {
        printf(" # Failed to open disk.out for writing: %d %s\n", errno, strerror(errno));
        return 0;
    }
    for (int i = 0; i < NUM_BLOCKS; i++) {
        dRead2(i, buf);
        fwrite(buf, 1, BLOCK_SIZE, dump);
    }
    if (fclose(dump) != 0) {
        printf(" # Failed to close disk.out; %d %s\n", errno, strerror(errno));
        return 0;
    }
    printf(" # Dumped disk contents to disk.out\n");
    return 0;
}
