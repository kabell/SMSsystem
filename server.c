/**
 * @file server.c
 * @author Michal Korbela, Dávid Horov
 * @date 13 May 2016
 * @brief Implementation of server for SMS system
 * @see https://github.com/kabell/SMSsystem
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>


#define SERVER_CAPACITY 1000    /**< Maximum number of users connected to server */
#define BUFFER_SIZE 1000        /**< Maximum size of buffer for everything - messages, usernames, passwords, queries*/


/**
 * Struct for storing users in memory
 */
typedef struct {
    char * username;
    char * password;
} user_t;

char * inPipe = "serverin";             /**< Name of named pipe for comunicating with server */

FILE * in = NULL;                       /**< Named pipe for server input */

char * serverLock = "server.lock";      /**< If exist file with this name, an instance of server is running */

user_t * users_logged[SERVER_CAPACITY]; /**< Array for storing logged users in memory */

/**
 * @brief Read password from stanard input with prompt 
 * @param message - Prompt
 * @param password - String for store password - memory must be allocated before
 *
 */
void getpassword(char * message, char * password)
{
    printf("%s\n",message);
    static struct termios oldt, newt;
    int i = 0;
    int c;

    //saving the old settings of STDIN_FILENO and copy settings for resetting
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;

    //setting the approriate bit in the termios struct
    newt.c_lflag &= ~(ECHO);

    //setting the new bits
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    //reading the password from the console
    while ((c = getchar())!= '\n' && c != EOF && i < BUFFER_SIZE){
        password[i++] = c;
    }
    password[i] = '\0';

    /*resetting our old STDIN_FILENO*/
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

}

/**
 *  @brief Send message to client
 *  @param user - Message will be send to this user
 *  @param message - Message
 *
 *  Message is written to named pipe of target client. Name of the named pipe is same as username.
 */
void message_send(user_t * user, char * message){
    
    //message is send to same pipe as username
    FILE * out = fopen(user->username,"w");
    fprintf(out,"%s",message);
    fclose(out);
}

/**
 * @brief Check if given credentials are correct
 * @param user - struct containing username and password for check
 * @return 1 if credentials are valid 0 otherwise
 *
 * Function opens a file with all users and passwords and then it is trying to find given user in the file with all registered users.
 * If we find a user we check if the password is the same as password given at registration if yes returns 1 else 0.
 */
int user_auth(user_t * user){

    //file contains all registered logins and passwords
    FILE * login = fopen("login","r");

    //if file doesn't exist - there are no valid logins and password - credentials are wrong
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


/**
 * @brief Login user to server
 * @param user - User to be logged
 * @return 1 if login was successful, 0 otherwise (server is full and there is no space for more users)
 *
 * Function finds a first free index in the array of users strucures, when it finds index we put that user to that index. If there is no free index, return 0.
 *
 */

//login user - add user to list of logged in users
int user_login(user_t * user){
    
    //find empty index
    int pos = 0;
    while(pos<SERVER_CAPACITY && users_logged[pos]){
        pos++;
    }

    //if there is no empty index - server is full
    if(pos==SERVER_CAPACITY)
        return 0;
   
    //put user to the empty space
    users_logged[pos] = user;

    //print message in server console
    printf("user %s logged in.\n",user->username);
    fflush(NULL);

    //send message to user - login OK
    message_send(user,"1");    
    return 1;

}

/**
 * @brief Logout user from server
 * @param name - Username for logout
 *
 * Function finds a user which is given as name in the array of users structures and then it deletes user from that structure and frees all allocated memory.
 * After successful logout, send message to client. Message is "Logged out.".
 *
 */
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

/**
 * @brief - Send all online users to client
 * @param pipename - Name of named pipe for output
 *
 * Write all online users to given named pipe. Pipe is named by pid of client process - which is unique. Usernames are separated by | (pipe) and added "- " before every username - for better reading at client side.
 *
 */
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

/**
 * @brief Request for server is parsed by this function
 * @param message - Request for server
 *
 * 
 * Format of request should be valid otherwise you probably get SEGFAULT
 *
 * There are 4 types of request
 *
 * 1. <b>Request for login</b><br/>
 * Format: <pre>1|username|password</pre>
 * Username and password are stored in memory. After that server is trying to authenticate user via user_auth() and log in user via user_login().<br/>
 * If credentials are invalid, or login was unsuccessful - client recieves a message about it by function message_send().
 *
 * 2. <b>Request for online users</b><br/>
 * Format: <pre>2|pipename</pre>
 * @see print_online
 *
 * 3. <b>Redirect message from client</b><br/>
 * Format: <pre>3|from|to|message</pre>
 * Server send message using function message_send() to client "to" of format "from" -> message
 *
 * 4. <b>Request for logout user</b><br/>
 * Format: <pre>4|username</pre>
 * Logout user with given username
 * @see user_logout
 *
 */

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
            //and log in
            if(!user_login(user)){
                message_send(user,"Server is full !!!\n");
            }
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

/**
 * @brief Terminate server
 *
 * At first all logged users are logged out (commands to quit clients are included). Then clean free all used memory, delete all used files and terminate server.
 */
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
    if(in)
        fclose(in);
    unlink(inPipe);
    remove(serverLock);
    //exit program
    exit(0);
}


/**
 *
 * @brief Initialize server
 *
 * Set terminate behaviour - when the server is asked to terminate, run server_quit() - for clean environment.<br/>
 * Create and open named pipe for server requests.<br/>
 * Prepare memory for logged users.
 *
 */
void server_init(){
    
    //add signal for quit when q is pressed in the another process
    signal(SIGTERM, server_quit);

    //create named pipe for server input
    mkfifo(inPipe,0666);

    //open named pipe for input
    in = fopen(inPipe, "r");
    
    //prepare memory for logged users
    for(int i=0; i<SERVER_CAPACITY; i++){
        users_logged[i] = NULL;
    }
}

/**
 *
 * @brief Process all requests in input
 * 
 * Requests for server are written to named pipe "serverin" one per line. So server read input line by line and process using this way all requests.
 * Queries are processed by server_parse_input()
 */
void server_run(){
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

/**
 *
 * @brief Main function
 *
 * Server has 2 modes:
 * 1. <b>Registration mode</b><br/>
 * If server is run with exactly 2 argiments. Second argument is considered as username for register. At first server check if the username isn't already registered. If no, server asks for password 3 times using getpassword(). If passwords are equal server write username and password to login file - each per line. So all odd lines contain usernames and even lines contain passwords - numbering from 1
 * 
 * 2. <b>Normal mode</b><br/>
 * At first server checks if there is no instance of server running. After server creates a file "server.lock" (used to prevent multiple instances of running servers) and server forks - parent process is waiting for commands from commandline, and the child process run server_init() and server_run()
 */



int main(int argc, char ** argv){

    //if is given parameters for add user
    if(argc>2){

        //check for username in login file
        FILE * login_file = fopen("login","r");
        if(login_file){
            //find given username in login file
            char username[BUFFER_SIZE],password[BUFFER_SIZE];
            while(fgets(username,BUFFER_SIZE,login_file)!=NULL){
                fgets(password,BUFFER_SIZE,login_file);
                username[strlen(username)-1]='\0';
                password[strlen(password)-1]='\0';

                //if we found suitable username and password credentials are OK
                if(!strcmp(argv[2],username)){
                    printf("Username already exists !!!!\n");
                    fclose(login_file);
                    return 0;
                }
            }
            fclose(login_file);
        }

        char password[BUFFER_SIZE],password1[BUFFER_SIZE];

        //ask for password
        getpassword("Password: ",password);
        //ask for password again
        getpassword("Retype password: ",password1);
        
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

    //check for running server
    FILE * f = fopen(serverLock,"r");
    if(f){
        printf("Another instance of server is running !!!\n");
        fclose(f);
        return 0;
    }

    //create server.lock
    f = fopen(serverLock,"w");
    fclose(f);



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
