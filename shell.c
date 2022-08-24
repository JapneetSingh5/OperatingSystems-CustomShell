#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

#define PATH_BUFFER_SIZE 256
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)<(b))?(b):(a))
#define INPUT_BUFFER_SIZE 128

char currentDirectory[PATH_BUFFER_SIZE];
char cmdHistory[5][128] = {"command1","command2","command3","command4","command5"};
char *input;
int processList[1024];
int processStatus[1024]; // 1 means running, 0 means stopped
int processNo = 0;
int commandNo = 0;
int isSetEnvCommand = 0;

//todo: ps_history and cmd_history fix

void signalHandler(int signal){
    exit(0);
}

int isPsHistory(char* input){
    if(strcmp(input,"ps_history")==0){
        return 1;
    }
    return 0;
}

int isCmdHistory(char* input){
    if(strcmp(input,"cmd_history")==0){
        return 1;
    }
    return 0;
}

int isPiped(char* input){
    for(int i=0; i<INPUT_BUFFER_SIZE; i++){
        if(input[i]==NULL || input[i]=='\0'){
            return 0;
        }else if(input[i]=='|'){
            return 1;
        }
    }
    return 0;
}

int isSetEnv(char* input){
    for(int i=0; i<INPUT_BUFFER_SIZE; i++){
        if(input[i]==NULL || input[i]=='\0'){
            return 0;
        }else if(input[i]=='='){
            return 1;
        }
    }
    return 0;
}

void execPsHistory(){
    for(int i=0; i<processNo+1; i++){
        printf("%d ",processList[i]);
        if(processStatus[i]==0){
            printf("STOPPED\n");
        }else if(waitpid(processList[i],NULL,WNOHANG)==0){
            printf("RUNNING\n");
        }else{
            processStatus[i]=0;
            printf("STOPPED\n");
        }
    }
}

void  execCmdHistory(){
    for(int i=(processNo - 1)%5; i>((processNo-1)%5 - MIN(5,processNo)); i--){
        printf("%s \n",cmdHistory[(i+5)%5]);
    }
}

// split builds the arg array
void split(char* input, char* args[128], char* delim){
    int i = 0;
    char *token = strtok(input, delim);
    while( token != NULL ) {
        if(token[0]=='&'){
            ++token;   
        }
        args[i]=token;
        i++;
        token = strtok(NULL, delim);
    }
    args[i]=NULL;
}

void pipeSplit(char* input, char* args[128], char* delim){
    int i = 0;
    char *token = strtok(input, delim);
    while( token != NULL ) {
        args[i]=token;
        i++;
        token = strtok(NULL, delim);
    }
    args[i]=NULL;
}

void buildPipeArgs(char* input, char *cmds[128]){
    char *token = strtok(input, "|");
    cmds[0]=token;
    for(int i=0; cmds[0][i] != '\0'; i++){
        if(cmds[0][i] == ' ' && cmds[0][i+1] == '\0'){
            cmds[0][i] = '\0';
            cmds[0][i+1] = NULL;
        }
    }
    // printf("CMD1 (%s) \n", token);
    token = strtok(NULL, "|");
    if(token[0]==' ' && token[1]!='\0'){
        ++token;
    }
    // printf("CMD2 (%s) \n", token);
    cmds[1]=token;
}

void printPrompt(){
    getcwd(currentDirectory,  PATH_BUFFER_SIZE);
    printf("%s~$ ",currentDirectory);
}

void getInput(){
    fgets(input, INPUT_BUFFER_SIZE, stdin);
    input=strsep(&input,"\n");
}

int main(){
    input = malloc(128*sizeof(char));
    char *args[128], *args2[128],  *envargs[128];
    signal(SIGINT, signalHandler);
    while(1){
        printPrompt();
        getInput();
        if(input[0]==NULL){
            printf("ERROR \n");
            continue;
        }
        char *inputToSplit1[INPUT_BUFFER_SIZE],*inputToSplit2[INPUT_BUFFER_SIZE], *inputToSplitPipeArgs[INPUT_BUFFER_SIZE];
        if(isPiped(input)){
            // INPUT IS PIPED
            // cmds[0] will store first command to be executed
            // cmds[1] will store the second command to be executed
            char* cmds[128];
            strcpy(inputToSplitPipeArgs,input);
            buildPipeArgs(inputToSplitPipeArgs, cmds);
            strcpy(inputToSplit1, cmds[0]);
            // build args array for cmds[0], stor in args
            pipeSplit(inputToSplit1, args, " ");
            // build args array for cmds[1], store in args2
            strcpy(inputToSplit2, cmds[1]);
            pipeSplit(inputToSplit2, args2, " ");
            // pipe file descriptors
            // pfd[0] : read side, pfd[1] : write side of the pipe
            int pfd[2];
            pipe(pfd);
            // create first child to execute first command
            pid_t childProcessID1 = fork();
            if(childProcessID1==0){
                // write end of the pipe becomes stdout 
                dup2(pfd[1], STDOUT_FILENO);	
                // close read end of the pipe
                close(pfd[0]); 		            
                close(pfd[1]);
                if(isPsHistory(cmds[0])){
                    execPsHistory();
                }else if(isCmdHistory(cmds[0])){
                    execCmdHistory();
                }else{
                    for(int i=0; args[i]!=NULL; i++){
                        if(args[i][0]=='$'){
                            char* token;
                            token = malloc(128 * sizeof(char));
                            strcpy(token, args[i]);
                            ++token;
                            args[i]=getenv(token);
                        }
                    }
                    int status = execvp(args[0], args);
                    if(status<0){
                        printf("ERROR \n");
                        break;
                    }
                };
                exit(0);
            }else if(childProcessID1>0){
                // wait for first child to get done
                waitpid(childProcessID1, NULL, 0); 
                processList[processNo]=childProcessID1;
                processStatus[processNo]=1;
                processNo++;
                pid_t childProcessID2 = fork();
                if(childProcessID2==0){
                    // read end of the pipe becomes stdin 
                    dup2(pfd[0], STDIN_FILENO);	
                    close(pfd[0]); 
                    // close write end of the pipe
                    close(pfd[1]); 		
                    for(int i=0; args2[i]!=NULL; i++){
                        if(args2[i][0]=='$'){
                            char* token;
                            token = malloc(128 * sizeof(char));
                            strcpy(token, args2[i]);
                            ++token;
                            args2[i]=getenv(token);
                        }
                    }
                    int status = execvp(args2[0], args2);
                    if(status<0){
                        printf("ERROR \n");
                        break;
                    }
                }else if(childProcessID2>0){
                    close(pfd[0]);
                    close(pfd[1]);
                    // wait for first child to get done
                    waitpid(childProcessID1, NULL, 0); 
                    // wait for second child to get done
                    waitpid(childProcessID2, NULL, 0); 
                    processList[processNo]=childProcessID2;
                    processStatus[processNo]=1;
                    processNo++;
                }else{
                    printf("ERROR \n");
                }
            }else if(childProcessID1<0){
                printf("ERROR \n");
            }
            strcpy(cmdHistory[commandNo%5],input);
            commandNo++;
        }else{
            // INPUT IS NOT PIPED
            strcpy(inputToSplit1,input);
            split(inputToSplit1,args," ");
            if(isSetEnv(input)){
                isSetEnvCommand = 1;
                char *inputToSplitEnv[INPUT_BUFFER_SIZE];
                strcpy(inputToSplitEnv,input);
                split(inputToSplitEnv,args,"=");
                setenv(args[0],args[1],1);
            }else{
                isSetEnvCommand = 0;
                int childProcessID = fork();
                if(childProcessID == 0){
                    // this is the child process 
                    // execute the command here
                    if(isCmdHistory(args[0])){
                        execCmdHistory();
                    }else if(isPsHistory(args[0])){
                        execPsHistory();
                    }else{
                        for(int i=0; args[i]!=NULL; i++){
                            if(args[i][0]=='$'){
                                char* token;
                                token = malloc(128 * sizeof(char));
                                strcpy(token, args[i]);
                                ++token;
                                args[i]=getenv(token);
                            }
                        }
                        execvp(args[0], args);
                        //if control reaches here, execvp returned an error
                        printf("ERROR \n");
                    }
                    exit(0);
                }else if(input[0]!='&'){
                    processList[processNo]=childProcessID;
                    processStatus[processNo]=1;
                    int childProcessStatus;
                    waitpid(childProcessID, &childProcessStatus, 0);
                }else if(input[0]=='&'){
                    processList[processNo]=childProcessID;
                    processStatus[processNo]=1;
                }
            }
            strcpy(cmdHistory[commandNo%5],input);
            if(!isSetEnvCommand) processNo++;
            commandNo++;
        }
    }
    return 0;
}