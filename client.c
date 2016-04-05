#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>


//size of each used buffer
#define BUFFER_SIZE 1000

char buffer[BUFFER_SIZE];

//name of the named pipe used by server
char * serverPipe = "serverin";

//username aand password
char username[BUFFER_SIZE],password[BUFFER_SIZE];

//stream for comuninacting with server
FILE * server;

//pid of the main process
int mypid = 0;

//infinity loop - waiting for new messages
void receive_messages(){
    
    //create named pipe for receiving messages
    mkfifo(username,0666);
    FILE * in = fopen(username,"r");

    //and checking for new messages
    while(1){

        //if there is a new message
        if(fgets(buffer,BUFFER_SIZE,in)!=NULL){

            //if message is command to exit program
            if(!strcmp(buffer,"Logged out.")){
                printf("Exiting...\n");
                //close all streams and named pipes and ask parent process to exit
                fclose(in);
                unlink(username);
                kill(mypid, SIGTERM);
                exit(0);
            }

            //print message
            printf("%s",buffer);
            fflush(stdout);
        }
        //this is optional, but prevent for heavy use of CPU
        usleep(10000);
    }
}

//login user to server
void login(){
    
    int logged = 0;
    int tries = 0;

    //while user is not logged in, try to repeat password
    while(!logged && tries<3){

        //get password
        printf("Password: ");
        scanf("%s",password);

        //send username and password to server
        server = fopen(serverPipe, "w");
        fprintf(server,"1|%s|%s",username,password);
        fclose(server);

        //create named pipe for receiving answer
        mkfifo(username,0666);
        FILE * in = fopen(username,"r");
        
        //read answer 
        fgets(buffer,BUFFER_SIZE,in);
        printf("Response: %s\n",buffer);
        
        //destroy named pipe
        fclose(in);
        unlink(username);

        //if username and password are correct
        if(strcmp(buffer,"1")==0){
            printf("Login OK.\n");
            logged = 1;
        }
        //if not
        else
            printf("Try again.\n");
        tries++;
    }
    if(!logged && tries==3){
        printf("Acces denied !!");
        exit(0);
    }
}

// ask server for online users and print them
void query_online(){
    
    //create a new pipe with name of pid process(it is unique)
    int pid = getpid();
    char pipe_name[10];
    sprintf(pipe_name,"%d",pid);
    mkfifo(pipe_name,0666);

    //send query to server
    server = fopen(serverPipe,"w");
    fprintf(server,"2|%s",pipe_name);
    fflush(server);
    fclose(server);

    //get response from server
    char result[BUFFER_SIZE];
    FILE * pipe = fopen(pipe_name,"r");
    fgets(result,BUFFER_SIZE,pipe);

    //replace all | for newlines
    for(int i=0; i<(int)strlen(result); i++){
        if(result[i]=='|')
            result[i]='\n';
    }

    //print online users
    printf("%s\n",result);

    //destroy pipe
    fclose(pipe);
    unlink(pipe_name);
}


//send message to another user
void query_send_message(){

    //ask for username
    printf("Message send to(write username): \n");
    char name[BUFFER_SIZE];
    fgets(name,BUFFER_SIZE,stdin);
    name[strlen(name)-1]='\0';
    
    //ask for message
    printf("Write message:\n");
    char message[BUFFER_SIZE];
    fgets(message,BUFFER_SIZE,stdin);
    message[strlen(message)-1]='\0';

    //send message to server
    server = fopen(serverPipe,"w");
    fprintf(server,"3|%s|%s|%s",username,name,message);
    fclose(server);

}


//logout user
void query_logout(){

    //send query for logout to server
    server = fopen(serverPipe,"w");
    fprintf(server,"4|%s",username);
    fclose(server);
}

//this is main process that comunicate with user
void main_process(){

    //at first start receiving messages
    int pid = fork();
    if (pid == (pid_t) 0){
        //childs process
        receive_messages();
    }
    else{
        //parents process

        //set pid as global variable - child process will terminate parents process in the end
        mypid = pid;

        //infinity loop for comunicating with user
        while(1){

            //print menu
            printf("Choose an option (Press [1-3]):\n1 - Display online users\n2 - Send message to username\n3 - Quit\n");
            
            //read option
            int mode;
            scanf("%d",&mode);
            getchar();

            //print onilne
            if(mode==1){
                query_online();
            }
            //send message
            else if(mode==2)
                query_send_message();
            //logout
            else if(mode==3)
                query_logout();
            //nothing happens
            else
                printf("Bad option\n"); 
        }
    }



}



int main(int argc, char ** argv){

    //help for run
    if(argc!=2){
        printf("Usage: ./client username\n");
        return 0;
    }

    //get username from argv
    strcpy(username,argv[1]);
    //login
    login();
    //run program
    main_process();
    return 0;
}
