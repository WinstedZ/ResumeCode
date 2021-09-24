#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

int fonly_mode=0; //Global variable for foreground only mode to be changed by signal handler

void handle_SIGTSTP(int sig_num){ //Function to handle the SIGTSTP signal that toggles foreground only mode
    if(fonly_mode==0){ //if foreground only mode is off
        char* display = "\nEntering foreground-only mode (& is now ignored)\n"; //create the message to tell the user
        write(STDOUT_FILENO,display,50); //tell the user that it is being turned on
        fonly_mode=1-fonly_mode; //toggle foreground-only mode
    }
    else if(fonly_mode==1){ //else if it's currently on
        char* undisplay = "\nExiting foreground-only mode\n"; //create the message to tell the user
        write(STDOUT_FILENO,undisplay,30); //tell the user that it is being turned off
        fonly_mode=1-fonly_mode; //toggle foreground-only mode
    }
}


int main(){
    char temp_line[2048]; //create an array to store the command line
    char line[3000]; //create another array for after calculating variable expansion
    int background_pid[200]; //create an array to hold all the PID's of bg processes
    int background_status[200]; //create an array to also hold their status'
    char** line_ptr; //create a pointer to the words of the line broken by spaces
    char** arg_ptr; //create a pointer to the commands of the line after shell redirect
    int fg_child_status=0; //create a variable to store the last foreground child's status
    int child_status=0; //create a variable to store the last background child's status
    int background_result; //create a variable for if statements regarding the background
    int scanf_result; //create a variable for if statements regarding the scanf input check
    int file_result; //create a variable to check if files open correctly
    int background; //create a background flag for the current command
    int background_counter=0; //create a counter for the last index of background processes
    int input = 0; //create a flag for whether the command sets input to a file
    int output =0; //create a flag for whether the command sets output to a file
    int i; //create a multipurpose counter for many things
    int j; //another multipurpose counter
    int counter; //another multipurpose counter
    int space_counter; //count the number of spaces in a line to get the words (sc+1)

    int pid = getpid(); //get the current pid
    char id[6]; //create an array for it to use in variable expansion
    sprintf(id,"%d",pid); //print the pid to the array

    struct sigaction SIGTSTP_action = {0}, SIGINT_action = {0}; //create the signal structs

    SIGTSTP_action.sa_handler = handle_SIGTSTP; //redirect the handler to custom function
    SIGINT_action.sa_handler = SIG_IGN; //redirect the handler to ignore

    sigaction(SIGTSTP, &SIGTSTP_action, NULL); //confirm changes
    sigaction(SIGINT, &SIGINT_action, NULL); //confirm changes



    for(i=0; i<200; i++){ //for all items in the background process arrays
        background_pid[i]=-5; //make the pid's an invalid value
        background_status[i]=-5; //make the status an invalid value
    }

    while(1){
        memset(temp_line,'\0',2048); //clear the line pre variable expansion
        memset(line,'\0',3000); //clear the lind post variable expansion
        setbuf(stdout, NULL); //set the stream to unbuffered to avoid fflush weirdness
        input=0; //clear the input flag
        output=0; //clear the output flag
        background=0; //clear the background flag
        i=0; //using i soon, so clear it
        while(i<background_counter){//check all the background processes
            if(background_pid[i]==-5){//if the spot is empty, skip it
            }
            else{//otherwise scan it
                background_result = waitpid(background_pid[i],&background_status[i],WNOHANG);//check to see if the child has terminated with WNOHANG
                if(background_result!=0){//if it has terminated and waitpid succeeded
                    printf("Background pid %d is done: ", background_pid[i]);//print the background with pid is done
                    if(WIFEXITED(background_status[i])){//if the child was exited normally
                        printf("Exit value %d\n",WEXITSTATUS(background_status[i])); //tell the user that status number
                    }
                    else if(WIFSIGNALED(background_status[i])){//or was it terminated by a signal
                        printf("Terminated by signal %d\n",WTERMSIG(background_status[i])); //tell the user that signal
                    }
                    background_pid[i]=-5;//clear it's spot
                    background_status[i]=-5;//clear it's spot
                    if(i==background_counter-1){//if it's the last one in the array (not the last total spot)
                        background_counter--; //then we can look in less places
                    }
                }
            }
            i++;//iterate i
            
        }
        printf(":"); //print the prompt
        scanf_result = scanf("%2047[^\n]",temp_line); //get the command that the user types in and place the return value into result
        getchar(); //getchar to avoid scanf issues
        if(scanf_result>0){ //if the user typed something
            i=0; //clear counter i
            j=0; //clear counter j
            space_counter=0; //clear the spaces in the command
            while(temp_line[i]!='\0'){ //while the line still has stuff in it
                if(temp_line[i]==' '){ //if the current character is a space
                    space_counter++; //increase the counter for spaces
                }
                if(temp_line[i]=='$' && temp_line[i+1]=='$'){ //if there is variable expansion
                    i++; //immediately prep i to skip the copy
                    counter=0; //clear another counter
                    while(counter<6){ //while the counter is in bounds
                        line[j]=id[counter]; //copy the PID into the new line
                        j++; //step the new line again
                        counter++; //step the PID char again
                    }
                }
                else{ //otherwise
                    line[j]=temp_line[i]; //just copy the characters from the pre-variable expansion to post-variable expansion array
                }
                i++; //iterate i
                j++; //iterate j
            }
            if(line[j-1]=='&'){ //if the user specified a background process
                background=1; //flag the background process
                space_counter--; //get rid of the access to the ampersand
            }
            line_ptr=malloc(sizeof(char**)*(space_counter+1));//initialize memory for the pointer to all the words
            arg_ptr=malloc(sizeof(char**)*(space_counter+1));//initialize memory for the pointer to the exec arguments
            i=0;//clear i
            while(i!=space_counter+1){//step to the end of the line
                if(i==0){//if the very first run
                    line_ptr[i] = strtok(line," ");//run strtok using line
                }
                else{
                    line_ptr[i] = strtok(NULL," ");//else use null instead
                }
                i++;//iterate i
            }
            line_ptr[i]=NULL;//make the last character NULL to signal the end
            if(line_ptr[0][0]=='#'){//if the user is commenting then do nothing
            }
            else if(strcmp(line_ptr[0],"cd")==0){//else if the user wants to change directory
                if((space_counter+1)==1){ //if there are zero spaces, so just cd
                    chdir(getenv("HOME")); //change directory to the home environment
                }
                else{
                    chdir(line_ptr[1]);//otherwise change directory to the specified directory
                }
            }
            else if(strcmp(line_ptr[0],"status")==0){//if the user wants the status of the last foreground child
                if(WIFEXITED(fg_child_status)){//if the child was exited normally
                    printf("Exit value %d\n",WEXITSTATUS(fg_child_status)); //tell the user that status number
                }
                else if(WIFSIGNALED(fg_child_status)){//or was it terminated by a signal
                    printf("Terminated by signal %d\n",WTERMSIG(fg_child_status)); //tell the user that signal
                }
            }
            else if(strcmp(line_ptr[0],"exit")==0){ //if the user wants to exit
                exit(0); //exit normally
            }
            else{ //otherwise this is where it starts getting messy
                pid_t child_pid = fork(); //fork the process!
                switch(child_pid){//switch statement for the child and parent logic
                    case -1: //if it fails!
                        perror("Hull Breach!\n");//print errors
                        exit(1);//exit with an error
                        break;//stop the switch

                    case 0: //if it's the child
                        SIGTSTP_action.sa_handler = SIG_IGN; //ignore the SIGTSTP signal
                        sigaction(SIGTSTP, &SIGTSTP_action, NULL); //confirm the change
                        if(background==0 || fonly_mode==1){ //if it's running in the foreground
                            SIGINT_action.sa_handler = SIG_DFL; //it needs to act normally for the SIGINT signal
                            sigaction(SIGINT, &SIGINT_action, NULL); //confirm changes
                        }
                        i=0; //reset multipurpose counter
                        j=0; //reset multipurpose counter
                        while(line_ptr[i]!=NULL){ //while we are in the line of words
                            if(strcmp(line_ptr[i],">")==0){ //if they are redirecting output
                                file_result = open(line_ptr[i+1], O_WRONLY | O_CREAT, 0777); //open the file or create it if it DNE
                                if(file_result!=-1){ //if the file continues to open
                                    if(dup2(file_result,1)==-1){ //if redirecting output fails
                                        perror("");//print the error
                                        exit(1); //exit
                                    }
                                    else{ //otherwise the file is good
                                        output=1; //flag that output has changed
                                    }
                                }
                                else{
                                    perror(""); //otherwise print the file not found error
                                    exit(1); //exit
                                }
                                i++; //skip the file name to open
                            }
                            else if(strcmp(line_ptr[i],"<")==0){ //if they are redirecting input from somewhere
                                file_result = open(line_ptr[i+1], O_RDONLY); //try to open the file
                                if(file_result!=-1){ //if the file continues to open
                                    if(dup2(file_result,0)==-1){ //try to set the permissions and if that doesn't work
                                        perror(""); //print the error
                                        exit(1); //exit
                                    }
                                    else{ //otherwise the file is good
                                        input=1; //flag that input has changed
                                    }
                                }
                                else{
                                    perror(""); //otherwise print the file not found error
                                    exit(1); //exit
                                }
                                i++;//skip the file name to open
                            }
                            else if(strcmp(line_ptr[i],"&")==0){//just in case, ignore any ampersands
                            }
                            else{
                                arg_ptr[j]=line_ptr[i]; //otherwise copy the word over to the exec argument list
                                j++; //step the argument list to the next
                            }
                            i++; //step the line words to the next
                        }
                        arg_ptr[j]=NULL; //set the last character in the argument list to NULL for exec
                        if((input==0||output==0)&&(background==1&&fonly_mode==0)){ //if the child is in the background but did not specify input/output
                            file_result = open("/dev/null", O_RDWR, 0777); //open /dev/null
                            if(input==0){ //if the input wasn't redirected
                                dup2(file_result,0); //change the input to /dev/null
                            }
                            if(output==0){ //if the output wasn't redirected
                                dup2(file_result,1); //change the output to /dev/null
                            }
                        }
                        i=0;//clear i
                        execvp(arg_ptr[0],arg_ptr);//call exec using the commmand arguments
                        perror(arg_ptr[0]); //if it fails print an error
                        exit(1); //and exit

                    default: //if it's the parent
                        if(background==0 || fonly_mode==1){ //if it's not in the background or the foreground only mode is specified
                            waitpid(child_pid, &fg_child_status, 0); //wait for the child to finish
                            if(WIFSIGNALED(fg_child_status)){ //if the child was killed by a signal
                                printf("\nTerminated by signal %d\n",WTERMSIG(fg_child_status));//tell the user about that signal
                            }
                        }
                        else{ //otherwise it's in the background
                            i=0; //clear i
                            while(background_pid[i]!=-5 || i<background_counter){ //look for the closest empty spot
                                i++; //step i
                            }
                            waitpid(child_pid, &child_status, WNOHANG); //wait with nohang for the child
                            background_pid[i]=child_pid; //set the pid into the background array
                            printf("Background pid is %d\n", child_pid); //print the pid to the user as starting in background
                            background_status[i]=child_status; //set the status into the bg status array
                            if(i==background_counter){ //if the space was a new one
                                background_counter++; //make access to the next space in case no spaces free up
                            }
                        }
                        break; //break out of the switch
                }
            }
        }
    }
    return 0;
}