#include "functions.h"

int main(int argc, char *argv[]){
    struct sigaction sa;
    char line[CMDLINELENGTH * 2];
    int i = 0;
    char *token = NULL;
    int stage = 0;
    int totalPipes = 0;
    int isPipe = 0;
    int prev[2], next[2], status;
    FILE *file;
    int invalid = 0;

    if(argc > 2){
        fprintf(stderr, "Usage: ./mush [filename]\n");
        exit(EXIT_FAILURE);
    }
    if(argc == 2){ /*file given*/
        file = fopen(argv[1], "r");
    }
    else{
        file = stdin;
    } 
    /*setup sigint handler*/
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    while(1){
        /*reset values*/
        stage = 0;
        totalPipes = 0;
        isPipe = 0;

        if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)){
            if(file == stdin){
                printf("8-P ");
            }
            fflush(stdout);
        }
        if(readline(file, line) && !feof(file)){
            if(strlen(line) > CMDLINELENGTH){
                fprintf(stderr, "command too long\n");
                continue;
            }
            /*count number of pipes*/
            for(i = 0; line[i] != '\0'; i++){
                if(line[i] == '|'){
                    totalPipes++;
                }
            }
            if(totalPipes >= CMDPIPELIM){
                fprintf(stderr, "pipeline too deep\n");
                continue;
            }
            token = strtok(line, "|\n");
            /*empty line*/
            if(token == NULL){
                continue;
            }
            /*remove space at end*/
            if(isspace(token[strlen(token) - 1])){
                token[strlen(token) - 1] = '\0';
            }
            /*no pipe*/
            if (totalPipes == 0){
                if(exec_stage(token, stage, -1, -1, totalPipes,
                              prev, next) == -1){
                    continue;
                }
                stage++;
            }
            /*first command before pipe*/
            else{
                /*create first pipe*/
                if(-1 == pipe(next)){
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
                if(exec_stage(token, stage, -1, stage + 1, totalPipes,
                              prev, next) == -1){      
                    continue;
                }
                stage++;
                isPipe = 1;
            }
            while((token = strtok(NULL, "|\n"))){
                /*remove space at end*/
                if(isspace(token[strlen(token) - 1])){
                    token[strlen(token) - 1] = '\0';
                }
                /*remove space at front*/
                if(isspace(token[0])){
                    token++;
                }
                if(strcmp(token, "") == 0){
                    fprintf(stderr, "invalid null command\n");
                    invalid = 1;
                    break;
                }
                /*last stage*/
                if(totalPipes == stage){
                    if(exec_stage(token, stage, stage - 1, -1, totalPipes,
                                  prev, next) == -1){
                        /*wait for all other processes to finish if parseerror*/
                        for(i = 0; i < totalPipes; i++){
                            if(wait(&status) == -1){
                                perror("wait");
                                continue;
                            }
                            /*print \n if child was SIGINT'd*/
                            if(WIFSIGNALED(status) && WTERMSIG(status) == SIGINT){
                                printf("\n");
                            }
                        }
                        invalid = 1;
                        break;
                    }
                    stage++;
                    isPipe = 0;
                }
                /*middle stage*/
                else{
                    if(exec_stage(token, stage, stage-1, stage+1, totalPipes,
                                  prev, next) == -1){
                        /*wait for all other processes to finish if parseerror*/
                        for(i = 0; i < stage; i++){
                            if(wait(&status) == -1){
                                perror("wait");
                                continue;
                            }
                            /*print \n if child was SIGINT'd*/
                            if(WIFSIGNALED(status) && WTERMSIG(status) == SIGINT){
                                printf("\n");
                            }
                        }
                        invalid = 1;
                        break;
                    }
                    stage++;
                }
            }
            if(invalid){
                invalid = 0;
                continue;
            }
            /*no cmd after pipe*/
            if(isPipe){
                fprintf(stderr, "invalid null command\n");
                continue;
            }
            /*close last one if not a single cmd*/
            if(stage > totalPipes && totalPipes != 0){
                close(prev[READ]);
                close(prev[WRITE]);
            }
            /*wait for children*/
            if(line[0] != 'c' || line[1] != 'd'){
                for(i = 0; i <= totalPipes; i++){
                    if(wait(&status) == -1){
                        perror("wait");
                        continue;
                    }
                    /*print \n if child was SIGINT'd*/
                    if(WIFSIGNALED(status) && WTERMSIG(status) == SIGINT){
                        printf("\n");
                    }
                }
            }
        }
        else {
            break;
        }
    }
    return 0;
}
