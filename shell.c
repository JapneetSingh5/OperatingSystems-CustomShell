#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

#define PATH_BUFFER_SIZE 256
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)<(b))?(b):(a))
#define INPUT_BUFFER_SIZE 128

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

int isPrintEnv(char* input){
    for(int i=0; i<INPUT_BUFFER_SIZE; i++){
        if(input[i]==NULL || input[i]=='\0'){
            return 0;
        }else if(input[i]=='$'){
            return 1;
        }
    }
    return 0;
}

// split builds the arg array
void split(char* input, char* args[128], char* delim){
    int i = 0;
    char *token = strtok(input, delim);
    while( token != NULL ) {
        if(token[0]=='&'){
            ++token;   
        }
        if(token[0]=='$'){
            printf("Token was earlier %s\n", token);
            ++token;
            token = getenv(token);
            printf("Token is now %s\n", token);
        }
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

int main(){
    char currentDirectory[PATH_BUFFER_SIZE];
    char input[INPUT_BUFFER_SIZE];
    char *args[128], *args2[128];
    char cmdHistory[5][128] = {"command1","command2","command3","command4","command5"};
    int processList[1024];
    signal(SIGINT, signalHandler);
    int processNo = 0;
    while(1){
        getcwd(currentDirectory,  PATH_BUFFER_SIZE);
        printf("%s~$ ",currentDirectory);
        scanf("%[^\n]%*c",input); // scanf with regex to take spaces as input and not exit
        char *inputToSplit1[INPUT_BUFFER_SIZE],*inputToSplit2[INPUT_BUFFER_SIZE];
        if(isPiped(input)){
            // piped
            printf("Command is piped -> %s\n",input);
            char* cmds[128];
            buildPipeArgs(input, cmds);
            printf("cmd1 %s , cmd2 %s", cmds[0], cmds[1]);
            int pfd[2];
            if(pipe(pfd)!=0){
                printf("ERROR \n");
            }
            printf("Here \n");
            strcpy(inputToSplit1, cmds[0]);
            split(inputToSplit1, args, " ");
            strcpy(inputToSplit2, cmds[1]);
            split(inputToSplit2, args2, " ");
            int childProcessID1 = fork();
            if(childProcessID1==0){
                dup2(pfd[1], 1);	/* this end of the pipe becomes the standard output */
                close(pfd[0]); 		/* this process don't need the other end */
                if(isSetEnv(cmds[0])){
                    char *inputToSplitEnv[INPUT_BUFFER_SIZE];
                    strcpy(inputToSplitEnv,cmds[0]);
                    split(inputToSplitEnv,args2,"=");
                    setenv(args2[0],args2[1],"=");
                }else{
                    execvp(args[0], args);
                }
            }else{
                int childProcessStatus;
                waitpid(childProcessID1, &childProcessStatus, 0);
                int childProcessID2 = fork();
                if(childProcessID2==0){
                    dup2(pfd[0], 0);	/* this end of the pipe becomes the standard input */
                    close(pfd[1]);		/* this process doesn't need the other end */
                    if(isSetEnv(cmds[1])){
                        char *inputToSplitEnv[INPUT_BUFFER_SIZE];
                        strcpy(inputToSplitEnv,cmds[1]);
                        split(inputToSplitEnv,args,"=");
                        setenv(args[0],args[1],"=");
                        strcpy(cmdHistory[processNo%5],input);
                    }else{
                        execvp(args2[0], args2);
                    }  
                }else{
                    waitpid(childProcessID2, &childProcessStatus, 0);
                }
            }
            printf("Here2 \n");
            strcpy(cmdHistory[processNo%5],input);
            processNo++;
        }else{
            // not piped
            strcpy(inputToSplit1,input);
            split(inputToSplit1,args," ");
            if(isPsHistory(args[0])){
            strcpy(cmdHistory[processNo%5],input);
            for(int i=0; i<processNo+1; i++){
                printf("%d ",processList[i]);
                int processStatus;
                if(waitpid(processList[i],&processStatus,WNOHANG)<=0){
                    printf("STOPPED\n");
                }else{
                    printf("RUNNING\n");
                }
            }
        }else if(isCmdHistory(args[0])){
            for(int i=(processNo - 1)%5; i>((processNo-1)%5 - MIN(5,processNo)); i--){
                printf("%s \n",cmdHistory[(i+5)%5]);
            }
            strcpy(cmdHistory[processNo%5],input);
        }else if(isSetEnv(input)){
            char *inputToSplitEnv[INPUT_BUFFER_SIZE];
            strcpy(inputToSplitEnv,input);
            split(inputToSplitEnv,args,"=");
            setenv(args[0],args[1],"=");
            strcpy(cmdHistory[processNo%5],input);
        }else{
            int childProcessID = fork();
            if(childProcessID == 0){
                // this is the child process 
                // execute the command here
                execvp(args[0], args);
                //if control reaches here, execvp returned an error
                printf("ERROR \n");
            }else if(input[0]!='&'){
                processList[processNo]=childProcessID;
                int childProcessStatus;
                waitpid(childProcessID, &childProcessStatus, 0);
                strcpy(cmdHistory[processNo%5],input);
                printf("Finished with child process %d status %d \n", childProcessID, childProcessStatus);
            }else if(input[0]=='&'){
                processList[processNo]=childProcessID;
                strcpy(cmdHistory[processNo%5],input);
                printf("Child running in bg %d \n",childProcessID);
                int ps;
                printf("%d - ",waitpid(childProcessID,&ps,WNOHANG));
            }
        }
            processNo++;
        }
    }
    return 0;
}