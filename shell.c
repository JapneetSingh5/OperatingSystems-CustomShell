#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>

#define PATH_BUFFER_SIZE 256
#define INPUT_BUFFER_SIZE 1024

void signalHandler(int signal){
    exit(0);
}

int main(){
    char currentDirectory[PATH_BUFFER_SIZE];
    char input[INPUT_BUFFER_SIZE];
    signal(SIGINT, signalHandler);
    while(1){
        getcwd(currentDirectory,  PATH_BUFFER_SIZE);
        printf("%s~$ ",currentDirectory);
        scanf("%[^\n]%*c",input); // scanf with regex to take spaces as input and not exit
        printf("%s\n",input);
    }
}
