# SMSsystem

1. Introduction

    This program allows sending messages between 2 or more clients.
    Messages are redirected and managed by a server.
    
    The key features are (see section 8 for full description): sending messages to multiple users; hidden passwords; multiple registration login not allowed;  one client console login allowed; self-messaging; create users while server is running. 

2. Compiling Requirements

    - gcc - version supporting standard gnu99
    - make 
    - unix like operating system
    - write access
    - doxygen (just for documentation)
    - graphviz (just for documentation)



3. How to Install

    - git clone https://github.com/kabell/SMSsystem.git
    - cd SMSsystem
    - make
    - make docs 

4. How to Uninstall
    
    - make clean

5. How to run the client/server application

    1. Run server:

        ./server

    2. Register users if they are not registered yet.

        ./server adduser <i>username<\i>
        
        <i>username<\i> is login for user.
        
       Then, you are asked to input the user's password and confirmed the password. If the two passwords are the 
        
        same, the user will be added to the server. You can add users while the server is running.

    3. Run as many clients as you want (limited by server capacity in server.c, which is 1000 users)

        ./client  <i>username<\i>

        The  <i>username<\i> is stored in the server, which must be registered before using it in the server.

        You have 3 attempts to type the password. After the third wrong attempt the program will end.


6. How to use the cilent/server application 

    1. Make sure that server is running

    2. When you log in as a client you have 3 options that describes what you can do:

        When you press:
        - 1 - You will see all users, which are actualy logged in;
        - 2 - You can send message to yourself or logged users (For more information check 7.a);
        - 3 - You will be logged out and the program will quit.

    3. To quit the server simply press 'q'+enter
        Server will log out all users and then quit
   

7. Features

    1. You can send message to more than one user. 
    Type logins of users and separate them by a space.
    
        For example:
        - Shell: Message send to(write username): 
        - User: login1 login2 login3 

    2. Hidden passwords.
    When user type a password to log in, you can not see how many characters his/her password has. This is for safety reasons.

    3. Multiple registration login not allowed.
    You can not register user with the same login and different password. Each user has a unique login.

    4. One client console login allowed.
    You can not log in the same user to more than one console. Like real SMS system (You do not have 2 sim cards for one number)
    
8. Restrictions:

    1. You can use only alphanumeric characters for username or password

    2. You can use all characters for messages except | (pipe)
    

9. Flow Diagram

    <img src="../schema.png"/>
        
 

