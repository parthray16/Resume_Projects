#include "functions.h"

int readline(FILE *file, char *line){
    int c = 0;
    int i = 0;
    if(file == stdin){
        if(fgets(line, CMDLINELENGTH * 2, file)){
            return 1;
        }
        else{
            return 0;
        }
    }
    else{
        while((c = getc(file)) != '\n'){
            if(c == EOF){
                return 0;
            }
            line[i++] = c;
        }
        if(i > CMDLINELENGTH * 2){
            line[CMDLINELENGTH * 2] = '\0';
        }
        else{
            line[i] = '\0';
        }
        return 1;
    }
}

int exec_stage(char *line, int stage, int in, int out,
                int totalPipes, int prev[], int next[]){
    struct sigaction sa;
    int inrd = 0;
    int outrd = 0;
    int i = 0;
    char copy[CMDLINELENGTH];
    char indest[CMDLINELENGTH] = "original stdin";
    char outdest[CMDLINELENGTH] = "original stdout";
    int infile = STDIN_FILENO;
    int outfile = STDOUT_FILENO;
    char *rest = line;
    char *cmd = NULL;
    char *token = NULL;
    int argc = 0;
    char *argv[ARGSLIM + 1];
    pid_t child;
    strcpy(copy, line);
    cmd = strtok_r(rest, " ", &rest);
    
    /*check cmd*/
    if ((strcmp(cmd, "<") == 0) || (strcmp(cmd, ">") == 0)){
        fprintf(stderr, "%s: not a valid command\n", cmd);
        return -1;
    }
    argc++;
    argv[0] = strdup(cmd);
    /*count redirections*/
    for(i = 0; copy[i] != '\0'; i++){
        if(copy[i] == '<'){
            inrd++;
        }
        if(copy[i] == '>'){
            outrd++;
        }
    }
    /*multiple redirections*/
    if(inrd > 1){
        fprintf(stderr, "%s: bad input redirection\n", cmd);
        /*free argv*/
        for(i = 0; i < argc; i++){
            free(argv[i]);
        }
        return -1;
    }
    if(outrd > 1){
        fprintf(stderr, "%s: bad output redirection\n", cmd);
        /*free argv*/
        for(i = 0; i < argc; i++){
            free(argv[i]);
        }
        return -1;
    }
    /*4 cases*/
    /*First Stage*/
    if(in == -1 && out == -1){ /*no pipe*/
        while((token = strtok_r(rest, " ", &rest))){
            if(strcmp(token, "<") == 0){
                token = strtok_r(rest, " ", &rest);
                if(token == NULL || strcmp(token, ">") == 0 
                                 || strcmp(token, "") == 0){
                    fprintf(stderr, "%s: bad input redirection\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                strcpy(indest, token);
            }
            else if(strcmp(token, ">") == 0){
                token = strtok_r(rest, " ", &rest);
                if(token == NULL || strcmp(token, "<") == 0 
                                 || strcmp(token, "") == 0){
                    fprintf(stderr, "%s: bad output redirection\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                strcpy(outdest, token);
            }
            else{
                argc++;
                if(argc - 1 > ARGSLIM){
                    fprintf(stderr, "%s: too many arguments\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc - 1; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                argv[argc - 1] = strdup(token);
            }
        }
        /*null terminate argv*/
        argv[argc] = NULL;
        /*cd execute*/
        if(strcmp(cmd, "cd") == 0){
            /*execute cd*/
            if(chdir(argv[1]) == -1){
                perror(argv[1]);
            }
            /*free argv*/
            for(i = 0; i < argc; i++){
                free(argv[i]);
            }
            return 0;
        }
        /*launch child*/
        if(!(child = fork())){
            /*child: rearrange input*/
            /*input redirection*/
            if(inrd > 0){
                if((infile = open(indest, O_RDONLY)) == -1){
                    perror(indest);
                    exit(EXIT_FAILURE);
                }
                if(-1 == dup2(infile, STDIN_FILENO)){
                    perror("dup2-in");
                    exit(EXIT_FAILURE);
                }
                close(infile);
            }
            /*child: rearrange output*/
            /*output redirection*/
            if(outrd > 0){
                if((outfile = open(outdest, O_WRONLY|O_TRUNC|O_CREAT,
                                     S_IRUSR|S_IWUSR)) == -1){
                    perror(outdest);
                    exit(EXIT_FAILURE);
                }
                if(-1 == dup2(outfile, STDOUT_FILENO)){
                    perror("dup2-out");
                    exit(EXIT_FAILURE);
                }
                close(outfile);
            }
            /*setup sigint handler*/
            sa.sa_handler = SIG_DFL;
            sa.sa_flags = 0;
            sigemptyset(&sa.sa_mask);
            if(sigaction(SIGINT, &sa, NULL) == -1){
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            /*execute cmd*/
            if(-1 == execvp(cmd, argv)){
                for(i = 0; i < argc; i++){
                    free(argv[i]);
                }
                perror(cmd);
                exit(EXIT_FAILURE);
            }
        }
        /*free argv*/
        for(i = 0; i < argc; i++){
            free(argv[i]);
        }
        return 0;
    }
    /*cannot have output redirection '>' if out is pipe*/
    if(in == -1 && out != -1){
        if(outrd > 0){
            fprintf(stderr, "%s: ambiguous output\n", cmd);
            /*free argv*/
            for(i = 0; i < argc; i++){
                free(argv[i]);
            }
            return -1;
        }
        while((token = strtok_r(rest, " ", &rest))){
            if(strcmp(token, "<") == 0){
                token = strtok_r(rest, " ", &rest);
                if(token == NULL || strcmp(token, ">") == 0 
                                 || strcmp(token, "") == 0){
                    fprintf(stderr, "%s: bad input redirection\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                strcpy(indest, token);
            }
            else{
                argc++;
                if(argc - 1 > ARGSLIM){
                    fprintf(stderr, "%s: too many arguments\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc - 1; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                argv[argc - 1] = strdup(token);
            }
        }
    }

    /*Middle Stage*/
    /*cannot have any redirection*/
    if(in != -1 && out != -1){
        if(inrd > 0){
            fprintf(stderr, "%s: ambiguous input\n", cmd);
            /*free argv*/
            for(i = 0; i < argc; i++){
                free(argv[i]);
            }
            return -1;
        }
        if(outrd > 0){
            fprintf(stderr, "%s: ambiguous output\n", cmd);
            /*free argv*/
            for(i = 0; i < argc; i++){
                free(argv[i]);
            }
            return -1;
        }
        while((token = strtok_r(rest, " ", &rest))){
            argc++;
            if(argc - 1 > ARGSLIM){
                fprintf(stderr, "%s: too many arguments\n", cmd);
                /*free argv*/
                for(i = 0; i < argc - 1; i++){
                    free(argv[i]);
                }
                return -1;
            }
            argv[argc - 1] = strdup(token);
        }
    }
    /*Last Stage*/
    /*cannot have input redirection '<' if in is pipe*/
    if(in != -1 && out == -1){
        if(inrd > 0){
            fprintf(stderr, "%s: ambiguous input\n", cmd);
            /*free argv*/
            for(i = 0; i < argc; i++){
                free(argv[i]);
            }
            return -1;
        }
        while((token = strtok_r(rest, " ", &rest))){
            if(strcmp(token, ">") == 0){
                token = strtok_r(rest, " ", &rest);
                if(token == NULL || strcmp(token, "<") == 0 
                                 || strcmp(token, "") == 0){
                    fprintf(stderr, "%s: bad output redirection\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                strcpy(outdest, token);
            }
            else{
                argc++;
                if(argc - 1 > ARGSLIM){
                    fprintf(stderr, "%s: too many arguments\n", cmd);
                    /*free argv*/
                    for(i = 0; i < argc - 1; i++){
                        free(argv[i]);
                    }
                    return -1;
                }
                argv[argc - 1] = strdup(token);
            }
        }
    }
    /*null terminate argv*/
    argv[argc] = NULL;
    /*cd check*/
    if(strcmp(cmd, "cd") == 0){
        fprintf(stderr, "%s: must be executed alone\n", cmd);
        /*free argv*/
        for(i = 0; i < argc; i++){
            free(argv[i]);
        }
        return -1;
    }
    /*launch children*/
    if(!(child = fork())){
        /*child: rearrange input*/
        if(stage > 0){
            /*input redirection*/
            if(inrd > 0){
                if((infile = open(indest, O_RDONLY)) == -1){
                    perror(indest);
                    exit(EXIT_FAILURE);
                }
                if(-1 == dup2(prev[READ], infile)){
                    perror("dup2-in");
                    exit(EXIT_FAILURE);
                }
            }
            /*no input redirection*/
            else if( -1 == dup2(prev[READ], STDIN_FILENO)){
                perror("dup2-in");
                exit(EXIT_FAILURE);
            }
            /*clean up*/
            close(prev[READ]);
            close(prev[WRITE]);
        }
        /*child: rearrange output*/
        if(stage < totalPipes){
            if( -1 == dup2(next[WRITE], STDOUT_FILENO)){
                perror("dup2-out");
                exit(EXIT_FAILURE);
            }
            /*clean up*/
            close(next[READ]);
            close(next[WRITE]);
        }
        if(stage == totalPipes){
            /*output redirection*/
            if(outrd > 0){
                if((outfile = open(outdest, O_WRONLY|O_TRUNC|O_CREAT, 
                                S_IRUSR|S_IWUSR)) == -1){
                    perror(outdest);
                    exit(EXIT_FAILURE);
                }
                if(-1 == dup2(outfile, STDOUT_FILENO)){
                    perror("dup2-out");
                    exit(EXIT_FAILURE);
                }
                /*clean up*/
                close(outfile);
            }        
        }
        /*setup sigint handler*/
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        if(sigaction(SIGINT, &sa, NULL) == -1){
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
        /*execute cmd*/
        if(-1 == execvp(cmd, argv)){
            for(i = 0; i < argc; i++){
                free(argv[i]);
            }
            perror(cmd);
            exit(EXIT_FAILURE);
        }
    }
    /*free argv in parent*/
    for(i = 0; i < argc; i++){
        free(argv[i]);
    }
    /*parent: create and close pipes*/
    if (stage > 0){
        close(prev[READ]);
        close(prev[WRITE]);
    }
    prev[READ] = next[READ];
    prev[WRITE] = next[WRITE];
    if(stage < totalPipes){
        if(-1 == pipe(next)){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    return 0;  
}