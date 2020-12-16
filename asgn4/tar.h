#ifndef tar
#define tar

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sysmacros.h>
#include <pwd.h>
#include <grp.h>
#include <arpa/inet.h>
#include <utime.h>

typedef struct Header{
                                /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char end[12];                 /* 500 */
                                  /* 512 */
}__attribute__((packed)) Header;

#define CHKBLANKS "        "    /* 8 blanks, no null */
#define TMAGIC   "ustar"        /* ustar and a null */
#define TMAGLEN  6
#define TVERSION "00"           /* 00 and no null */
#define TVERSLEN 2
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define SYMTYPE  '2'            /* sym link */
#define DIRTYPE  '5'            /* directory */
#define BLOCKSIZE 512

void fillHeader(char *path, Header *header, int strictness);
int writeFile(int arfile, Header *header, char *file);
int tnwDir(const char *path, int arfile, int verbosity, int strictness);
void createArchive(char *filename, char *path[],
                    int numpaths, int verbosity, int strictness);
void printEntry(Header *header);
void extract_permissions(char permissions[], int mode);
void listArchive(char *filename, char *path[],
                    int numpaths, int verbosity, int strictness);
void createDir(Header *, int, char *);
void createSym(Header *, int, char *, int);
void createFile(Header *, int , char *);
void extractArchive(char *filename, char *path[],
                    int numpaths, int verbosity, int strictness);
char *getPathName(Header *header);
int checkHeader(Header *header);
int checkPaths(char *currpath, char *path[], int numpaths);
int checkCurSS(char *currpath, char *path[], int numpaths);
uint32_t extract_special_int(char *where, int len);
int insert_special_int(char *where, size_t size, int32_t val);
#endif
