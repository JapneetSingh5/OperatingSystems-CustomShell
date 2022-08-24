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

// split builds the arg array
void split(char* input, char* args[128], char* delim){
    int i = 0;
    char *token = strtok(input, delim);
    while( token != NULL ) {
        if(token[0]=='&'){
            ++token;   
        }
        // if(token[0]=='$'){
        //     // printf("Token was earlier %s\n", token);
        //     ++token;
        //     token = getenv(token);
        //     // printf("Token is now %s\n", token);
        // }
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

int main(){
    char currentDirectory[PATH_BUFFER_SIZE];
    char *input;
    input = malloc(128*sizeof(char));
    char *args[128], *args2[128],  *envargs[128];
    char cmdHistory[5][128] = {"command1","command2","command3","command4","command5"};
    int processList[1024];
    char **envVarNames;
    char **envVarValues;
    envVarNames = malloc(1024 * sizeof(char*));
    envVarValues = malloc(1024 * sizeof(char*));
    for(int i = 0; i < 1024; i++) {
        envVarNames[i] = malloc((128 + 1) * sizeof(char));
    }
    envVarValues = malloc(1024 * sizeof(char*));
    for(int i = 0; i < 1024; i++) {
        envVarValues[i] = malloc((128 + 1) * sizeof(char));
    }
    int envVarCount = 0;
    signal(SIGINT, signalHandler);
    int processNo = 0;
    while(1){
        // printf("\n envVarCount: %d , %s, %s\n", envVarCount, envVarNames[envVarCount-1], envVarValues[envVarCount-1]);
        getcwd(currentDirectory,  PATH_BUFFER_SIZE);
        printf("%s~$ ",currentDirectory);
        fgets(input, INPUT_BUFFER_SIZE, stdin);
        // input[strlen(input)-2]='\'
        input=strsep(&input,"\n");
        // scanf("%[^\n]%*c",input); // scanf with regex to take spaces as input and not exit
        if(input[0]=='\0'){
            printf("ERROR \n");
            continue;
        }
        char *inputToSplit1[INPUT_BUFFER_SIZE],*inputToSplit2[INPUT_BUFFER_SIZE];

        if(isPiped(input)){
            // piped
            strcpy(cmdHistory[processNo%5],input);

            char* cmds[128];
            buildPipeArgs(input, cmds);

            strcpy(inputToSplit1, cmds[0]);
            pipeSplit(inputToSplit1, args, " ");

            strcpy(inputToSplit2, cmds[1]);
            pipeSplit(inputToSplit2, args2, " ");
            // printf("%s, %s", args2[0], args2[1]);

            printf("cmd1 %s , cmd2 %s \n", cmds[0], cmds[1]);
            //cmds[0] is cmd1, cmds[1] is cmd2

            //pipe file descriptors
            //pfd[0] : read side, pfd[1] : write side of the pipe
            int pfd[2];
            pipe(pfd);

            pid_t childProcessID1 = fork();

            if(childProcessID1==0){
                dup2(pfd[1], STDOUT_FILENO);	// write end of the pipe becomes stdout 
                close(pfd[0]); 		            // close read end of the pipe
                close(pfd[1]);
                if(isSetEnv(cmds[0])){
                    char *argsDef[][128] = {"echo","done",NULL };
                    execvp("echo",argsDef);
                }else if(isPsHistory(cmds[0])){
                    for(int i=0; i<processNo+1; i++){
                        printf("%d ",processList[i]);
                        int processStatus;
                        if(waitpid(processList[i],&processStatus,WNOHANG)<=0){
                            printf("STOPPED\n");
                        }else{
                            printf("RUNNING\n");
                        }
                    }
                }else{
                    int status = execvp(args[0], args);
                    if(status<0){
                        printf("ERROR \n");
                        break;
                    }
                };
                exit(0);
            }else if(childProcessID1>0){
                waitpid(childProcessID1, NULL, 0); //wait for first child to get done
                if(isSetEnv(cmds[0])){
                    // printf("Setting env var in cmd1 \n");
                    char *inputToSplitEnv[INPUT_BUFFER_SIZE];
                    strcpy(inputToSplitEnv,cmds[0]);
                    pipeSplit(inputToSplitEnv,envargs,"=");
                    strcpy(envVarNames[envVarCount], envargs[0]);
                    strcpy(envVarValues[envVarCount], envargs[1]);
                    envVarCount++;
                }
                pid_t childProcessID2 = fork();
                if(childProcessID2==0){
                    dup2(pfd[0], STDIN_FILENO);	// read end of the pipe becomes stdin 
                    close(pfd[0]); 
                    close(pfd[1]); 		// close write end of the pipe
                    if(isSetEnv(cmds[1])){
                        char *inputToSplitEnv[INPUT_BUFFER_SIZE];
                        strcpy(inputToSplitEnv,cmds[1]);
                        pipeSplit(inputToSplitEnv,envargs,"=");
                        strcpy(envVarNames[envVarCount], envargs[0]);
                        strcpy(envVarValues[envVarCount], envargs[1]);
                        envVarCount++;
                        // setenv(envargs[0],envargs[1],1);
                        // strcpy(cmdHistory[processNo%5],input);
                    }else{
                        for(int i=0; args2[i]!=NULL; i++){
                            if(args2[i][0]=='$'){
                                // printf("Switching args $$\n");
                                char* token;
                                token = malloc(128 * sizeof(char));
                                // printf("Here %s\n", token);
                                strcpy(token, args2[i]);
                                // printf("token is %s, %s \n", token, args2[i]);
                                ++token;
                                // printf("token  to switch %s \n", token);
                                for(int j=0; j<envVarCount; j++){
                                    if(strcmp(token,envVarNames[j])==0){
                                        // printf("token switched to %s \n", envVarValues[j]);
                                        args2[i]=envVarValues[j];
                                        break;
                                    }
                                }
                            }
                        }
                        int status = execvp(args2[0], args2);
                        if(status<0){
                            printf("ERROR \n");
                            break;
                        }
                    };
                }else if(childProcessID2>0){
                    close(pfd[0]);
                    close(pfd[1]);
                    waitpid(childProcessID1, NULL, 0); //wait for first child to get done
                    waitpid(childProcessID2, NULL, 0); //wait for first child to get done
                }else{
                    printf("ERROR \n");
                }

            }else if(childProcessID1<0){
                printf("ERROR \n");
            }
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
                strcpy(envVarNames[envVarCount], args[0]);
                strcpy(envVarValues[envVarCount], args[1]);
                envVarCount++;
                // setenv(args[0],args[1],1);
                strcpy(cmdHistory[processNo%5],input);
            }else{
                int childProcessID = fork();
                if(childProcessID == 0){
                    // this is the child process 
                    // execute the command here
                    for(int i=0; args[i]!=NULL; i++){
                        if(args[i][0]=='$'){
                            printf("Switching args $$\n");
                            char* token;
                            token = malloc(128 * sizeof(char));
                            printf("Here %s\n", token);
                            strcpy(token, args[i]);
                            printf("token is %s, %s \n", token, args[i]);
                            ++token;
                            printf("token  to switch %s \n", token);
                            for(int j=0; j<envVarCount; j++){
                                if(strcmp(token,envVarNames[j])==0){
                                    printf("token switched to %s \n", envVarValues[j]);
                                    args[i]=envVarValues[j];
                                    break;
                                }
                            }
                        }
                    }
                    printf("%s, %s \n",args[0],args[1]);
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
                    // printf("Child running in bg %d \n",childProcessID);
                    int ps;
                    // printf("%d - ",waitpid(childProcessID,&ps,WNOHANG));
                }
            }
            processNo++;
        }
        input[0] = "\0";
    }
    return 0;
}