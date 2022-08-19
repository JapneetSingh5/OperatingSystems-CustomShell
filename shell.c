#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

#define PATH_BUFFER_SIZE 256
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)<(b))?(b):(a))
#define INPUT_BUFFER_SIZE 1024
#define BUILT_IN_COMMANDS_SIZE = 2;

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

void split(char* input, char* args[512]){
    int i = 0;
    char * token = strtok(input, " ");
    while( token != NULL ) {
        if(token[0]=='&'){
            ++token;
            
        }
        args[i]=token;
        i++;
        token = strtok(NULL, " ");
    }
    args[i]=NULL;
}

int main(){
    char currentDirectory[PATH_BUFFER_SIZE];
    char input[INPUT_BUFFER_SIZE];
    char *args[512];
    char cmdHistory[5][512] = {"command1","command2","command3","command4","command5"};
    int processList[1024];
    signal(SIGINT, signalHandler);
    int processNo = 0;
    while(1){
        getcwd(currentDirectory,  PATH_BUFFER_SIZE);
        printf("%s~$ ",currentDirectory);
        scanf("%[^\n]%*c",input); // scanf with regex to take spaces as input and not exit
        char *inputToSplit[INPUT_BUFFER_SIZE];
        strcpy(inputToSplit,input);
        split(inputToSplit, args);
        if(isPsHistory(args[0])){
            strcpy(&cmdHistory[processNo%5],input);
            processNo++;
            for(int i=0; i<processNo; i++){
                printf("%d ",processList[i]);
                int processStatus;
                if(waitpid(processList[i],&processStatus,WNOHANG)<=0){
                    printf("STOPPED\n");
                }else{
                    printf("RUNNING\n");
                }
            }
            // printf("Printing Ps history");
        }else if(isCmdHistory(args[0])){
            for(int i=(processNo - 1)%5; i>((processNo-1)%5 - MIN(5,processNo)); i--){
                printf("%s \n",cmdHistory[(i+5)%5]);
            }
            strcpy(&cmdHistory[processNo%5],input);
            processNo++;
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
                strcpy(&cmdHistory[processNo%5],input);
                processNo++;
                printf("Finished with child process %d status %d \n", childProcessID, childProcessStatus);
            }else if(input[0]=='&'){
                processList[processNo]=childProcessID;
                strcpy(&cmdHistory[processNo%5],input);
                processNo++;
                printf("Child running in bg %d \n",childProcessID);
                int ps;
                printf("%d - ",waitpid(childProcessID,&ps,WNOHANG));
            }
        }
    }
    return 0;
}
