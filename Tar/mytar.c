#include "tar.h"
#include "safe_mem_functions.h"
int main(int argc, char *argv[]){
    int i = 0;
    int j = 3;
    char *chr;
    char *filename = NULL;
    char **paths = NULL;
    int c = 0;
    int t = 0;
    int x = 0;
    int v = 0;
    int S = 0;
    int f = 0;
    if(argc < 3){
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    for(chr = argv[1]; *chr != '\0'; chr++){
        if(*chr == 'c'){
            c++;
        }
        else if(*chr == 't'){
            t++;
        }
        else if(*chr == 'x'){
            x++;
        }
        else if(*chr == 'v'){
            v++;
        }
        else if(*chr == 'f'){
            f++;
        }
        else if(*chr == 'S'){
            S++;
        }
        else{
            fprintf(stderr, "Invalid Option: %c\n", *chr);
            fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
            exit(EXIT_FAILURE);
        }
    }
    if (c > 1 || t > 1 || x > 1 || v > 1 || f > 1 || S > 1){
        fprintf(stderr, "Repeated modifiers\n");
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    if (f != 1){
        fprintf(stderr, "Must have f modifier\n");
        fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
        exit(EXIT_FAILURE);
    }
    filename = argv[2];
    if((argc - 3) > 0){
        paths = safe_malloc((argc - 3) * sizeof(char *));
    }
    while (i < (argc - 3)){
        paths[i++] = argv[j++]; 
    }
    if (c == 1){
        createArchive(filename, paths, argc - 3, v, S);
    }
    if (t == 1){
        listArchive(filename, paths, argc - 3, v, S);
    }
    if (x == 1){
        extractArchive(filename, paths, argc - 3, v, S);
    }
    if(paths != NULL){
        free(paths);
    }
    return 0;
}