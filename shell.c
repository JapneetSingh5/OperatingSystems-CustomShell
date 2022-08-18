#include<stdio.h>
#include<unistd.h>

#define PATH_BUFFER_SIZE 256
#define INPUT_BUFFER_SIZE 1024

int main(){
    char currentDirectory[PATH_BUFFER_SIZE];
    char input[INPUT_BUFFER_SIZE];
    while(1){
        getcwd(currentDirectory,  PATH_BUFFER_SIZE);
        printf("%s~$ ",currentDirectory);
        scanf("%s",input);
        printf("%s\n",input);
    }
}