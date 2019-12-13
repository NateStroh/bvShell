#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main (int argc, char *argv[]){
  while(1){
    //setting up a buffer to read in current directory and input
    char cwd[1024];
    char buff[1024];
    char *buffP = buff;

    //getting current directory
    getcwd(cwd, 1024);

    //array of char pointers that point to the commands/args in the input 
    char *debArg[512];
    char **p = debArg;
    
    //printing current directory, then dollar sign
    write(1, cwd, strlen(cwd));
    write(1, " $ ", 3);
  
    //reading in all the input and if its "\n" just skip doing anything to it
    if(read(0, buff, 1024) == 1){
      continue;
    }
    
    //pre-parsing for input/output redirection
    char *preParse = buff;
    char input[512];
    char output[512];
    int writingInput = 0;
    int appending = 0;
    int writingOutput = 0;
    int inputCount = 0;
    int outputCount = 0;
    while(*preParse != '\n'){
      //checking for input redirection
      if(*preParse == '<'){
        writingInput = 1;
        writingOutput = 0;
        *preParse = ' '; 
      }
      //checking for output redirection
      if(*preParse == '>'){
        //checking if previous item was > so we know whether to append or replace
        //appending
        if(*(preParse+1) == '>'){
          appending = 1;
        }
        writingInput = 0;
        writingOutput = 1;
        *preParse = ' '; 
      }
      //this should catch everything thats not getting fed into input/output
      else{
        if(*preParse != ' '){
          //write char to input buffer 
          if(writingInput == 1){
            input[inputCount] = *preParse;
            inputCount++;
            *preParse = ' '; 
          }
          if(writingOutput == 1){
            output[outputCount] = *preParse;
            outputCount++;
            *preParse = ' '; 
          }
        }
      }
      //advance pointer
      preParse++;
    }

    //parsing the input into different cmds/agrgs and setting pointers to them
    int debArgsIndex = 0;
    int prevSpace = 0;
    while(1){
      if(*buffP == '\n'){
        *buffP = '\0';
        break;
      }
      if(*buffP == ' '){
        if(prevSpace == 0){
          *buffP = '\0';
          prevSpace = 1;
        }
        else{
          prevSpace = 1;
        }
      }
      else{
        if(prevSpace == 1 || buffP == buff){
          *p = buffP;
          p++;
          debArgsIndex++;
        }
        prevSpace = 0;
      }
      buffP++;
    }

    debArg[debArgsIndex] = '\0';

    //if the command is cd use chdir 
    if(strcmp(debArg[0], "cd") == 0){
      if(chdir(debArg[1]) == -1){
        char *err = strerror(errno);
        write(2, err, strlen(err));
        write(1, "\n", 1);
      } 
    }
    //if command is exit
    else if(strcmp(debArg[0], "exit") == 0){
      exit(0);
    }
    //if command is pwd
    else if(strcmp(debArg[0], "pwd") == 0){
      write(1, cwd, strlen(cwd));
      write(1, "\n", 1);
    }
    //if command is debugargs
    else if(strcmp(debArg[0], "debugargs") == 0){
      write(1, "cmd: [debugargs]\n", 17);
      for(int i=0; i<debArgsIndex; i++){
        write(1, "arg: [", 6);
        write(1, debArg[i], strlen(debArg[i]));
        write(1, "]\n", 2);
      }
      write(1, "arg: [(null)]\n", 14);
    }
    else{
      int childPid;

      if((childPid = fork()) == 0){
        //if we are redirecting input
        if(inputCount > 0){
          close(0);
          if(open(input, O_RDONLY, S_IRWXU) == -1){
            char *err1 = strerror(errno);
            write(2, err1, strlen(err1));
            write(2, "\n", 1);
          }
        }
        //if we are redirecting output
        if(outputCount > 0){
          close(1);
          //if we are appending or not
          if(appending == 0){          
            if(open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU) == -1){
              char *err2 = strerror(errno);
              write(2, err2, strlen(err2));
              write(2, "\n", 1);
            }
          }
          else{
            if(open(output, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU) == -1){
              char *err3 = strerror(errno);
              write(2, err3, strlen(err3));
              write(2, "\n", 1);
            }
          }
        }
        if(execvp(debArg[0], debArg) == -1){
          char *err4 = strerror(errno);
          write(2, err4, strlen(err4));
          write(2, "\n", 1);
          exit(-1);
        }
      }
      else{
        while(wait(0) != childPid);
      }
    }
  }
  return 0;
}
