#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<signal.h>

#define SERVER_CAPACITY 10
#define BUFFER_SIZE 1000


//struct for storing users
typedef struct {
    char * username;
    char * password;
} user_t;

//name of named pipe for comunicating with server
char * inPipe = "serverin";

//named pipe for input
FILE * in;

//logged users
user_t * users_logged[SERVER_CAPACITY];


//send any message to user
void message_send(user_t * user, char * message){
    
    //message is send to same pipe as username
    FILE * out = fopen(user->username,"w");
    fprintf(out,"%s",message);
    fclose(out);
}

//authorize user
int user_auth(user_t * user){

    //file contained all accepted logins and password
    FILE * login = fopen("login","r");

    //if file doesn't exist - no valid logins and password - credentials are wrong
    if(!login)
        return 0;

    //find given username and password in login file
    char username[BUFFER_SIZE],password[BUFFER_SIZE];
    while(fgets(username,BUFFER_SIZE,login)!=NULL){
        fgets(password,BUFFER_SIZE,login);
        username[strlen(username)-1]='\0';
        password[strlen(password)-1]='\0';
        
        //if we found suitable username and password credentials are OK
        if(!strcmp(user->username,username) && !strcmp(user->password,password))
            return 1;
    }
    //we didn't find given login and password
    return 0;
}


//login user - add user to list of logged in users
void user_login(user_t * user){
    
    //find empty index
    int pos = 0;
    while(users_logged[pos]){
        pos++;
    }
   
    //put user to the empty space
    users_logged[pos] = user;

    //print message in server console
    printf("user %s logged in.\n",user->username);
    fflush(NULL);

    //send message to user - login OK
    message_send(user,"1");    

}

//logout user - delete user from list of logged in users
void user_logout(char * name){

    //browse all logged users
    for(int i=0; i<SERVER_CAPACITY; i++){

        //if username is same as username to logout
        if(users_logged[i] && !strcmp(name,users_logged[i]->username)){

            //send message to client - logged out
            message_send(users_logged[i],"Logged out.");

            //write message to server console
            printf("User %s logged out.\n",name);

            //free used memory and delete user from list of logged in users
            free(users_logged[i]->username);
            free(users_logged[i]->password);
            free(users_logged[i]);
            users_logged[i]=NULL;
        }
    }
}

//return all logged in users to user
void print_online(char * pipename){

    //name of named pipe is given, so open it
    FILE * pipe = fopen(pipename,"w");
    
    //list all logged in users
    for(int i=0; i<SERVER_CAPACITY; i++){
        //if this is logged in user, print him to the named pipe
        if(users_logged[i])
            fprintf(pipe,"|- %s",users_logged[i]->username);
    }
    
    //print newline and close the pipe
    fprintf(pipe,"\n");
    fclose(pipe);
}

//given query/message from user, so decide what to do
void server_parse_input(char * message){

    //if query is empty... do nothing
    if(strlen(message)==0)return;
    
    //if query is type 1 - login user
    if(message[0]=='1'){

        //skip first 2 characters "1|"
        message+=2;
        
        //alocate memory for user
        user_t * user = malloc(sizeof(user_t));
        
        //copy username
        char * separator = strchr(message,'|');
        separator[0]='\0';
        user->username = malloc((strlen(message)+1)*sizeof(char));
        strcpy(user->username, message);

        //erase copied characters from message
        message = separator+1;

        //copy password
        user->password = malloc((strlen(message)+1)*sizeof(char));
        strcpy(user->password, message);

        //authentificate used
        if(user_auth(user)){
            //and log in him
            user_login(user);
        }
        //wrong credentials
        else{
            message_send(user,"Login incorrect\n");
        }

    }


    //if query is of type 2 - send online users
    else if(message[0]=='2'){
        //print to server console
        printf("Sending online users\n");

        //skip first 2 characters - 2|
        message+=2;
        //call function with given name of named pipe
        print_online(message);
    }

    //query is of type 3 - send message
    //query format is 3|from|to|message
    else if(message[0]=='3'){
        
        //skip first 2 characters - 3|
        message+=2;

        //copy "from"
        char * separator = strchr(message,'|');
         separator[0]='\0';
        char * from = malloc((strlen(message)+1)*sizeof(char));
        strcpy(from, message);

        //erase copyied characters
        message = separator+1;

        //copy "to"
        separator = strchr(message,'|');
        separator[0]='\0';
        char * to = malloc((strlen(message)+1)*sizeof(char));
        strcpy(to, message);
        //erase copied characters
        message = separator+1;
        
        //find user "to"
        user_t * user=NULL;
        for(int i=0; i<SERVER_CAPACITY; i++){
            if(users_logged[i] && !strcmp(users_logged[i]->username,to)){
                user=users_logged[i];
            }
        }  

        //if we have found user "to"
        if(user){

            //compose message
            char tmp[BUFFER_SIZE];
            tmp[0]='\0';
            strcat(tmp,from);
            strcat(tmp," -> ");
            strcat(tmp,message);
            strcat(tmp,"\n");
            //send message to user "to"
            message_send(user,tmp);

            //print message to server console
            printf("User %s sent a message to %s\n",from, to);
        }

        //free used memory
        free(from);
        free(to);

    }

    //query is of type 4 - logout user
    else if(message[0]=='4'){

        //erase first 2 characters - 4|
        message+=2;
        //logout user
        user_logout(message);
    }

}


//initialize server
void server_init(){

    //create named pipe for server input
    mkfifo(inPipe,0666);

    //open named pipe for input
    in = fopen(inPipe, "r");
    
    //prepare memory for logged users
    for(int i=0; i<SERVER_CAPACITY; i++){
        users_logged[i] = NULL;
    }
}

//quit server - send message to all logged users - to force log out
void server_quit(){

    for(int i=0; i<SERVER_CAPACITY; i++){
        // is here is logged user
        if(users_logged[i]){
            //send message to him
            message_send(users_logged[i],"Server terminated\n");
            message_send(users_logged[i],"Logged out.");
            
            //free used memory for him
            free(users_logged[i]->username);
            free(users_logged[i]->password);
            free(users_logged[i]);
            users_logged[i]=NULL;
        }
    }
    //wait for deliver all messages
    sleep(1);
    //destroy named pipe
    fclose(in);
    unlink(inPipe);
    //exit program
    exit(0);
}

//run server - waiting for queries from pipe
void server_run(){

    //add signal for quit when q is pressed in the another process
    signal(SIGTERM, server_quit);

    //processign queries
    char buffer[BUFFER_SIZE];
    while(1){
        if(fgets(buffer,BUFFER_SIZE,in)!=NULL){
            server_parse_input(buffer);
        }

        //prevent for heavy use of CPU - optional
        usleep(10000);
    }
}





int main(int argc, char ** argv){

    //if is given parameters for add user
    if(argc>2){
        char password[BUFFER_SIZE],password1[BUFFER_SIZE];
        
        //ask for password
        printf("Password: ");
        scanf("%s",password);

        //ask for password again
        printf("Retype password: ");
        scanf("%s",password1);
        
        //if passwords are same
        if(!strcmp(password,password1)){
            //add username and password to login file
            FILE * f = fopen("login","a");
            fprintf(f,"%s\n",argv[2]);
            fprintf(f,"%s\n",password);
            fclose(f);
        }
        //passwords are not same
        else
            printf("Passwords are different!!!\n");

        //exiting
        return 0;
    }

    //print message to server console
    printf("To quit press q and then enter.\n");

    //create child process for processing queries
    int mypid = 0;
    int pid = fork();
    
    if(pid > (pid_t)0){
        //child process
        mypid = pid;
        server_init();
        server_run();
    }
    else{
        //main process
        while(1){
            char c;
            scanf("%c",&c);
            //if q is pressed terminate child process - processing queries
            if(c=='q'){
                printf("Are you sure you want to end the server?\n");
                printf("Please confirm (y/n): ");
                scanf("%s",&c);
                if(c=='y'){
                    kill(mypid,SIGTERM);
                    exit(0);
                }
            }
        }
    }
    //exit
    return 0;

}
