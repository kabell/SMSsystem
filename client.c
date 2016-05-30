/**
 * @file client.c
 * @author Michal Korbela, Dvid Horov
 * @date 13 May 2016
 * @brief Implementation of client for SMS system
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


#define BUFFER_SIZE 1000            /**< Size of buffer for everything */


char * serverPipe = "serverin";     /**< Name of the named pipe used by server as input */

char * serverLock = "server.lock";  /**< Name of the file indicates if server is running */

char username[BUFFER_SIZE];         /**< Username of client */
char password[BUFFER_SIZE];         /**< Password of client */

FILE * server;                      /**< Named pipe for communicating with server */

int mypid = 0;                      /**< Pid of parent process */

/**
 * @brief Recieve requests from server
 *
 * Function is waiting for new messages in infinity loop, if the message is command to exit the program,
 * function closes input file and unlinks the pipe and then sends SIGTERM signal to parents process.
 *
 */

void receive_messages(){
    
    //buffer for input
    char buffer[BUFFER_SIZE];
    
    //create named pipe for receiving messages
    mkfifo(username,0666);
    FILE * in = fopen(username,"r");

    //and checking for new messages
    while(1){

        //if there is a new message
        if(fgets(buffer,BUFFER_SIZE,in)!=NULL){

            //if message is a command to exit program
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

/**
 * @brief Read password from stanard input with prompt 
 * @param message - Prompt
 *
 * Function stores password to global variable password
 */

void getpassword(char * message)
{
    printf("%s\n",message);
    static struct termios oldt, newt;
    int i = 0;
    int c;

    //saving the old settings of STDIN_FILENO and copy settings for reseting
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;

    //setting the approriate bit in the termios struct
    newt.c_lflag &= ~(ECHO);          

    //setting new bits
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    //reading the password from the console
    while ((c = getchar())!= '\n' && c != EOF && i < BUFFER_SIZE){
        password[i++] = c;
    }
    password[i] = '\0';

    /*reseting our old STDIN_FILENO*/ 
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

}

/**
 * @brief Login user to server
 *
 *  Function tries to log in user in the loop of maximum 3 tries
 *  Firstly function loads password via function getpassword() and sends it to the server and waits for the answer recieved in pipe.
 *  Then destroys and unlinks used pipe. In the end of the loop function checks if the username and password are correct and finish, if not
 *  user has 2 more attempts for log in, if user fails after third attempt acces is denied and program ends.
 */
void login(){
    
    int logged = 0;
    int tries = 0;
    
    //buffer for input
    char buffer[BUFFER_SIZE];
 
    //while user is not logged in, try to repeat password
    while(!logged && tries<3){

        //get password
        getpassword("Password: ");

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
        if(strcmp(buffer,"Login OK")==0){
            logged = 1;
        }
        //if not
        else
            printf("Try again.\n");
        tries++;
    }
    if(!logged && tries==3){
        printf("Acces denied !!\n");
        exit(0);
    }
}

/**
 *
 * @brief Ask server for all online users
 *
 * Function creates a pipe with the name of PID of actual process, then sends a query to the server in format:<br/>
 * <pre>2|pipename</pre>
 * After that function is waiting for the response from the server. Response (message) is in format "login1|login2|login3..." so function replace all 
 * "|" characters to "\n" character. Then function prints the result and closes and unlinks the pipe.
 *
 */


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

/**
 *
 * @brief Send message to user(s)
 *
 * Function asks for username - send "TO". Then asks for a message.
 * If there ia not just 1 username, but multiple usernames separated by exactly one space, function parses whole line with delimiter-space, and sends message to all these users.
 * Request server to send message to given users is in following format:
 * <pre>3|from|to|message</pre> For every username separated by space.
 */

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

    //parse username

    char * newname = name;
    char * pos=strchr(newname,' ');
    while(1){
        if(pos!=NULL)
            pos[0]='\0';
        //send message to server
        server = fopen(serverPipe,"w");
        fprintf(server,"3|%s|%s|%s\n",username,newname,message);
        fclose(server);
        if(pos==NULL)
            break;
        newname=pos+1;
        pos=strchr(newname,' ');
    }

}


/**
 *
 * @brief Ask server for logout
 *
 * Functions sends a request  to server via pipe that user wants to log out. The format of the query is <pre>4|userNameToLogout</pre>
 *
 */
void query_logout(){

    //send query for logout to server
    server = fopen(serverPipe,"w");
    fprintf(server,"4|%s",username);
    fclose(server);
}

/**
 *
 * @brief Run client
 *
 * Functions creates 2 processes.
 *  - Child proces is recieving messages via calling function receive_messages()
 *  - Parents process writes to the console options for user then loads choosen option.
 *      1. If option is 1 process calls function query_online() to list all logged in users
 *      2. If option is 2 process calls function query_send_message() to send message to users
 *      3. If option is 3 process calls function query_logout() to log out actual user
 *      4. Else process informs user about bad option.
 *      
 *      Parents process is in the loop so after finishing one option user is asked again to choose an option and again,..
 *
 */



void client_run(){

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


/**
 *
 * @brief Main
 *
 * 1. Check if username is given as argument.
 * 2. Check if server is running.
 * 3. Try to login user via login()
 * 4. Run client_run()
 *
 */


int main(int argc, char ** argv){

    //help for run
    if(argc!=2){
        printf("Usage: ./client username\n");
        return 0;
    }

    //check for server running
    
    FILE * f = fopen(serverLock,"r");
    if(!f){
        printf("Server is not running !!!!\n");
        return 0;
    }
    fclose(f);
        



    //get username from argv
    strcpy(username,argv[1]);
    
    //check for already logged user
    if(access( username, F_OK ) != -1){
        printf("User already loggen in\n");
        return 0;
    }
    
    //login
    login();
    //run program
    client_run();
    return 0;
}
