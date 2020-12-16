#include "tar.h"
#include "safe_mem_functions.h"

void fillHeader(char *path, Header *header, int strictness){
    int i = 0;
    struct stat statbuf;
    int pathsize = 0;
    int type = 0;
    struct group *gs;
    struct passwd *ps;
    uint32_t sum = 0;
    if (lstat(path, &statbuf) == -1){
        perror("file nonexistent");
        exit(EXIT_FAILURE);
    }
    /*set name and prefix*/
    pathsize = strlen(path);
    if(pathsize > 256){
        perror("path too long");
        exit(EXIT_FAILURE);
    }
    if(pathsize > 100){
        for(i = pathsize - 100; i < pathsize; i++){
            if(path[i] == '/'){
                memset(header->prefix, '\0', 155 *sizeof(char));
                memset(header->name, '\0', 100 *sizeof(char));
                memcpy(header->prefix, path, i * sizeof(char));
                memcpy(header->name, path+i+1, (pathsize-i) * sizeof(char));
                break;
            }
        }
    }
    else{
        memset(header->name, '\0', 100 *sizeof(char));
        memset(header->prefix, '\0', 155 *sizeof(char));
        memcpy(header->name, path, pathsize * sizeof(char));      
    }
    /*set mode*/
    sprintf(header->mode, "%07o", 0x0FFF & statbuf.st_mode);
    /*set uid and gid*/
    if (strictness == 1){
        insert_special_int(header->uid, 8*sizeof(char), statbuf.st_uid);
        insert_special_int(header->gid, 8*sizeof(char), statbuf.st_gid);
    }
    else{
        sprintf(header->uid, "%07o", statbuf.st_uid);
        sprintf(header->gid, "%07o", statbuf.st_gid);
    }
    /*set mtime*/
    sprintf(header->mtime, "%011lo", statbuf.st_mtime);
    /*set typeflag*/
    type = statbuf.st_mode & S_IFMT;
    if(type == S_IFLNK){
        header->typeflag = SYMTYPE; 
    }
    else if(type == S_IFDIR){
        header->typeflag = DIRTYPE; 
    }
    else if(type == S_IFREG){
        header->typeflag = REGTYPE; 
    }
    else{
        header->typeflag = AREGTYPE; 
    }
    /*set size*/
    sprintf(header->size, "%011lo",
            (type == S_IFLNK || type == S_IFDIR)? 0: statbuf.st_size);
    /*set linkname*/
    if (type == S_IFLNK){
        if (readlink(path, header->linkname, 100 *sizeof(char)) == -1){
            perror("readlink");
        }
    }
    else{
        memset(header->linkname, '\0', 100 *sizeof(char));
    }
    /*set magic*/
    sprintf(header->magic, "%s", TMAGIC);
    /*set version*/
    header->version[0] = '0';
    header->version[1] = '0';
    /*set uname*/
    ps = getpwuid(statbuf.st_uid);
    sprintf(header->uname, "%.31s", ps->pw_name);
    if(strlen(ps->pw_name) < 32){
        memset(header->uname + strlen(ps->pw_name), '\0',
                32 - strlen(ps->pw_name));
    }
    else{
        header->uname[31] = '\0';
    }
    /*set gname*/
    gs = getgrgid(statbuf.st_gid);
    sprintf(header->gname, "%.31s", gs->gr_name);
    if(strlen(gs->gr_name) < 32){
        memset(header->gname + strlen(gs->gr_name), '\0',
                32 - strlen(gs->gr_name));
    }
    else{
        header->gname[31] = '\0';
    }
    /*set devmajor and devminor*/
    memset(header->devmajor, '\0', 8 * sizeof(char));
    memset(header->devminor, '\0', 8 * sizeof(char));
    /*set end*/
    memset(header->end, '\0', 12 * sizeof(char));
    /*set chksum*/
    memcpy(header->chksum, CHKBLANKS, 8);
    for(i = 0; i < BLOCKSIZE; i++){
        sum = sum + (unsigned char)header->name[i];
    }
    sprintf(header->chksum, "%07o", sum);
    return;
}

int writeFile(int arfile, Header *header, char *file){
    struct stat statbuf;
    int fd = 0;
    char* buff = NULL;
    int r_status = 0; 
    if(write(arfile, header, BLOCKSIZE) == -1){
        perror("write");
        return 0;
    }
    /* write file contents */
    if (lstat(file, &statbuf) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    fd = open(file, O_RDONLY);
    if (fd == -1){
        perror(file);
        exit(EXIT_FAILURE);
    }
    buff = safe_calloc(BLOCKSIZE, sizeof(char));
    for(r_status = read(fd, buff, BLOCKSIZE); r_status != 0;
        r_status = read(fd, buff, BLOCKSIZE)){
        if(write(arfile, buff, BLOCKSIZE) == -1){
            perror("writing file contents");
            return 0;
        }
        free(buff);
        buff = safe_calloc(BLOCKSIZE, sizeof(char));
    }
    free(buff);
    if (close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int tnwDir(const char *path, int arfile, int verbosity, int strictness){
    DIR *directory = NULL;
    struct dirent *entry;
    char newpath[256];
    Header header;

    if((directory = opendir(path)) == NULL){
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    for(entry = readdir(directory); entry != NULL; entry = readdir(directory)){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        if(entry->d_type == DT_DIR){
            sprintf(newpath, "%s%s/", path, entry->d_name);
            if (verbosity == 1){
                printf("%s\n", newpath);
            }
            fillHeader(newpath, &header, strictness);
            if(write(arfile, &header, BLOCKSIZE) == -1){
                perror("header write");
                exit(EXIT_FAILURE);
            }
            tnwDir(newpath, arfile, verbosity, strictness);
        }
        else if(entry->d_type == DT_LNK){
            sprintf(newpath, "%s%s", path, entry->d_name);
            if (verbosity == 1){
                printf("%s\n", newpath);
            }
            fillHeader(newpath, &header, strictness);
            if(write(arfile, &header, BLOCKSIZE) == -1){
                perror("write");
            }
        }
        else if(entry->d_type == DT_REG){
            sprintf(newpath, "%s%s", path, entry->d_name);
            if (verbosity == 1){
                printf("%s\n", newpath);
            }
            fillHeader(newpath, &header, strictness);
            writeFile(arfile, &header, newpath);
        }
    }
    if(closedir(directory) == -1){
        perror("closedir");
    }
    return 0;
}

void createArchive(char *filename, char *path[],
                    int numpaths, int verbosity, int strictness){
    char *eoa = NULL;
    char newpath[256];
    int arfile = 0;
    int i = 0;
    struct stat statbuf;
    Header header;
    arfile = open(filename, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
    if (arfile == -1){
        perror(filename);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < numpaths; i++){
        if (lstat(path[i], &statbuf) == -1){
            perror("file nonexistent");
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)){
            sprintf(newpath, "%s/", path[i]);
            if (verbosity == 1){
                printf("%s/\n", path[i]);
            }
            fillHeader(newpath, &header, strictness);
            if(write(arfile, &header, BLOCKSIZE) == -1){
                perror("write");
            }
            tnwDir(newpath, arfile, verbosity, strictness);
        }
        if (S_ISREG(statbuf.st_mode)){
            if (verbosity == 1){
                printf("%s\n", path[i]);
            }
            fillHeader(path[i], &header, strictness);
            writeFile(arfile, &header, path[i]);
        }
        if (S_ISLNK(statbuf.st_mode)){
            if (verbosity == 1){
                printf("%s\n", path[i]);
            }
            fillHeader(path[i], &header, strictness);
            if(write(arfile, &header, BLOCKSIZE) == -1){
                perror("write");
            }
        }
    }
    eoa = safe_calloc(BLOCKSIZE * 2, sizeof(char));
    if(write(arfile, eoa, BLOCKSIZE * 2) == -1){
        perror("write");
        exit(EXIT_FAILURE);
    }
    if (close(arfile) == -1){
        perror("arfile");
        exit(EXIT_FAILURE);
    }
    free(eoa);
    return;
}

void printEntry(Header *header){
    char permissions[11];
    struct tm *info;
    char owner_group[18];
    int size;
    char mtime[17];
    time_t value;
    permissions[10] = '\0';
    extract_permissions(permissions, strtol(header->mode, NULL, 8));
    if(header->typeflag == DIRTYPE){
        permissions[0] = 'd';
    }
    else if(header->typeflag == SYMTYPE){
        permissions[0] = 'l';
    }
    else{
        permissions[0] = '-';
    }
    snprintf(owner_group, 18, "%s/%s", header->uname, header->gname);
    size = strtol(header->size, NULL, 8);
    value = strtol(header->mtime, NULL, 8);
    info = localtime(&value);
    strftime(mtime, 17, "%Y-%m-%d %H:%M", info);
    printf("%s %s %8d %s ", permissions, owner_group, size, mtime);
    return;
}

void extract_permissions(char permissions[], int mode){
    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
}

void listArchive(char *filename, char *path[],
                    int numpaths, int verbosity, int strictness){
    int size = BLOCKSIZE;
    int fd = 0;
    int r_status = 0;
    Header header;
    char *currpath;
    fd = open(filename, O_RDONLY);
    if (fd == -1){
        perror(filename);
        exit(EXIT_FAILURE);
    }
    r_status = read(fd, &header.name, BLOCKSIZE);
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    while(r_status > 0){
        if(header.name[0] == '\0'){
            r_status = read(fd, &header.name, BLOCKSIZE);
            if(r_status == -1){
                perror("read tar");
                exit(EXIT_FAILURE);
            }
            if(header.name[0] == '\0'){
                break;
            }
            else{
                if (lseek(fd, -(BLOCKSIZE), SEEK_CUR) == -1){
                    perror("lseek");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if(checkHeader(&header) == 1){
            perror("bad header");
            exit(EXIT_FAILURE);
        }
        currpath = getPathName(&header);
        if(numpaths > 0){
            /*curr name does not match any path*/
            if(checkPaths(currpath, path, numpaths) == 0){ 
                size = strtol(header.size, NULL, 8);
                while(size > 0){
                    r_status = read(fd, &header.name, BLOCKSIZE);
                    if(r_status == -1){
                        perror("read tar");
                        exit(EXIT_FAILURE);
                    }
                    size -= BLOCKSIZE;
                }
                r_status = read(fd, &header.name, BLOCKSIZE);
                if(r_status == -1){
                    perror("read tar");
                    exit(EXIT_FAILURE);
                }
                free(currpath);
                continue;
            }  
        }
        if(verbosity == 1){
            printEntry(&header);
            printf("%s\n", currpath);
        }
        else{
            printf("%s\n", currpath);
        }
        /*go to next header*/
        size = strtol(header.size, NULL, 8);
        while(size > 0){
            r_status = read(fd, &header.name, BLOCKSIZE);
            if(r_status == -1){
                perror("read tar");
                exit(EXIT_FAILURE);
            }
            size -= BLOCKSIZE;
        }
        r_status = read(fd, &header.name, BLOCKSIZE);
        if(r_status == -1){
            perror("read tar");
            exit(EXIT_FAILURE);
        }
        free(currpath);
    }
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    if(close(fd) == -1){
        perror("close tar");
        exit(EXIT_FAILURE);
    }
}
void createDir(Header *header, int arfile, char *path){
    int r_status = 0;
    struct utimbuf times;
    int mode = 0;
    mode = strtol(header->mode, NULL, 8);
    if((mode & S_IXUSR) || (mode & S_IXGRP) || (mode & S_IXOTH)){
        mkdir(path, 
    S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
    }
    else{
        mkdir(path, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    } 
    r_status = read(arfile, header->name, BLOCKSIZE);
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    times.modtime = strtol(header->mtime, NULL, 8);
    times.actime = strtol(header->mtime, NULL, 8);
    utime(path, &times);
    return;
}
void createSym(Header *header, int arfile, char *path, int verbosity){
    int r_status = 0;
    struct utimbuf times;
    if (symlink(header->linkname, path) == -1){
        perror("link error");
        exit(EXIT_FAILURE);
    }
    r_status = read(arfile, header->name, BLOCKSIZE);
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    times.modtime = strtol(header->mtime, NULL, 8);
    times.actime = strtol(header->mtime, NULL, 8);
    utime(path, &times);
    return;
}
void createFile(Header *header, int arfile, char *path){
    int mode = 0;
    struct utimbuf times;
    int fd = 0;
    int size = BLOCKSIZE;
    int r_status = 0;
    int modtime = 0;
    modtime = strtol(header->mtime, NULL, 8);
    mode = strtol(header->mode, NULL, 8);
    if((mode & S_IXUSR) || (mode & S_IXGRP) || (mode & S_IXOTH)){
        fd = open(path, O_WRONLY|O_TRUNC|O_CREAT,
    S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
        if(fd == -1){
            perror("open file");
        }
    }
    else{
        fd = open(path, O_WRONLY|O_TRUNC|O_CREAT,
        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH); 
        if(fd == -1){
            perror("open file");
        }
    }
    size = strtol(header->size, NULL, 8);
    while(size > 0){
        r_status = read(arfile, &header->name, BLOCKSIZE);
        if(r_status == -1){
            perror("reading tar");
            exit(EXIT_FAILURE);
        }
        size -= BLOCKSIZE;
        if(size < 0){
            if(write(fd, &header->name, BLOCKSIZE - (-1*size)) == -1){
                perror("writing file contents");
                exit(EXIT_FAILURE);
            }
        }
        else{
            if(write(fd, &header->name, BLOCKSIZE) == -1){
                perror("writing file contents");
                exit(EXIT_FAILURE);
            }
        }
    }
    r_status = read(arfile, &header->name, BLOCKSIZE);
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    if(close(fd) == -1){
        perror("close file");
    }
    times.modtime = modtime;
    times.actime = modtime;
    utime(path, &times);
    return;
}
void extractArchive(char *filename, char *path[],
                    int numpaths, int verbosity, int strictness){
    int size = BLOCKSIZE;
    int fd = 0;
    int r_status = 0;
    char *currpath;
    Header header;
    fd = open(filename, O_RDONLY);
    if (fd == -1){
        perror(filename);
        exit(EXIT_FAILURE);
    }
    r_status = read(fd, &header.name, BLOCKSIZE);
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    while(r_status > 0){
        if(header.name[0] == '\0'){
            r_status = read(fd, &header.name, BLOCKSIZE);
            if(r_status == -1){
                perror("read tar");
                exit(EXIT_FAILURE);
            }
            if(header.name[0] == '\0'){
                break;
            }
            else{
                if (lseek(fd, -(BLOCKSIZE), SEEK_CUR) == -1){
                    perror("lseek");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if(checkHeader(&header) == 1){
            perror("bad header");
            exit(EXIT_FAILURE);
        }
        currpath = getPathName(&header);
        if(numpaths > 0){
            /*curr name is a subset of name give*/
            if(checkCurSS(currpath, path, numpaths) == 1){
                if(header.typeflag == DIRTYPE){
                    createDir(&header, fd, currpath);
                    free(currpath);
                    continue;
                }
                size = strtol(header.size, NULL, 8);
                while(size > 0){
                    r_status = read(fd, &header.name, BLOCKSIZE);
                    if(r_status == -1){
                        perror("read tar");
                        exit(EXIT_FAILURE);
                    }
                    size -= BLOCKSIZE;
                }
                r_status = read(fd, &header.name, BLOCKSIZE);
                if(r_status == -1){
                    perror("read tar");
                    exit(EXIT_FAILURE);
                }
                free(currpath);
                continue;
            }
            /*curr name does not match any path*/
            else if(checkPaths(currpath, path, numpaths) == 0){ 
                size = strtol(header.size, NULL, 8);
                while(size > 0){
                    r_status = read(fd, &header.name, BLOCKSIZE);
                    if(r_status == -1){
                        perror("read tar");
                        exit(EXIT_FAILURE);
                    }
                    size -= BLOCKSIZE;
                }
                r_status = read(fd, &header.name, BLOCKSIZE);
                if(r_status == -1){
                    perror("read tar");
                    exit(EXIT_FAILURE);
                }
                free(currpath);
                continue;
            }        
        }
        if(verbosity == 1){
            printf("%s\n", currpath);
        }
        if(header.typeflag == DIRTYPE){
            createDir(&header, fd, currpath);
        }
        else if(header.typeflag == SYMTYPE){
            createSym(&header, fd, currpath, verbosity);
        }
        else if(header.typeflag == REGTYPE || header.typeflag == AREGTYPE){
            createFile(&header, fd, currpath);
        }
        else{
            perror("invalid type");
        }
        free(currpath);
    }
    if(r_status == -1){
        perror("read tar");
        exit(EXIT_FAILURE);
    }
    if(close(fd) == -1){
        perror("close tar");
        exit(EXIT_FAILURE);
    }
}

char *getPathName(Header *header){
    char *path = malloc(256 * sizeof(char));
    char *name;
    if(strlen(header->prefix) == 0){
        memcpy(path, header->name, 100);
        path[100] = '\0';
        return path;
    }
    if(strlen(header->prefix) > 155){
        perror("path too long");
        exit(EXIT_FAILURE);
    }
    memcpy(path, header->prefix, 155);
    path[155] = '\0';
    name = malloc(101 * sizeof(char));
    memcpy(name, header->name, 100);
    name[100] = '\0';
    sprintf(path, "%s/%s", path, name);
    free(name);
    return path;
}

int checkHeader(Header *header){
    u_int32_t sum = 0;
    int i = 0;
    for(i = 0; i < BLOCKSIZE; i++){
        if(i >= 148 && i < 156){
            sum += (unsigned char) ' ';
        }
        else{
            sum = sum + (unsigned char)header->name[i];
        }
    }
    if(strtol(header->chksum, NULL, 8) != sum){
        return 1;
    }
    return 0;
}

int checkPaths(char *currpath, char *path[], int numpaths){
    int i = 0;
    for(i = 0; i < numpaths; i++){
       if(strstr(currpath, path[i]) != NULL){
           return 1;
       } 
    }
    return 0;
}

int checkCurSS(char *currpath, char *path[], int numpaths){
    int i = 0;
    for(i = 0; i < numpaths; i++){
       if((strcmp(path[i], currpath) != 0) &&
            (strstr(path[i], currpath) != NULL)){
           return 1;
       } 
    }
    return 0;
}

uint32_t extract_special_int(char *where, int len) {
/* For interoperability with GNU tar. GNU seems to
* set the high–order bit of the first byte, then
* treat the rest of the field as a binary integer
* in network byte order.
* I don’t know for sure if it’s a 32 or 64–bit int, but for
* this version, we’ll only support 32. (well, 31)
* returns the integer on success, –1 on failure.
* In spite of the name of htonl(), it converts int32 t
*/
int32_t val= -1;
if ( (len >= sizeof(val)) && (where[0] & 0x80)) {
/* the top bit is set and we have space
* extract the last four bytes */
    val = *(int32_t *)(where+ len - sizeof(val));
    val = ntohl(val); /* convert to host byte order */
}
return val;
}

int insert_special_int(char *where, size_t size, int32_t val){
/* For interoperability with GNU tar. GNU seems to
* set the high–order bit of the first byte, then
* treat the rest of the field as a binary integer
* in network byte order.
* Insert the given integer into the given field
* using this technique. Returns 0 on success, nonzero
* otherwise
*/
int err=0;

if(val < 0 || (size < sizeof(val))){
    err++;
}else{
memset(where, 0, size); /* Clear out the buffer */
*(int32_t *)(where+size-sizeof(val)) = htonl(val); /* place the int */
*where |= 0x80; /* set that high–order bit */
}
return err;
}