# SMSsystem

1. Introduction

    This simple program you can use for sending messages between 2 or more clients.

2. Requirements

    - gcc - version supporting standard gnu99
    - make 
    - unix like operating system
    - write access
    - doxygen (just for documentation)
    - graphviz (just for documentation)



3. Install

    - git clone https://github.com/kabell/SMSsystem.git
    - cd SMSsystem
    - make
    - make docs 

4. Uninstall
    
    - make clean

5. How to run

    1. At first run server

        ./server

    2. Register users, if they are not registered yet.

        ./server adduser "username"
        
        where username is login for user
        
        After that you will be asked 2 times for password of this user. If passwords are the 
        
        same user will be added to server. You can add users when server is running.

    3. Then you can run how many clients you want (limited by server capacity in server.c)

        ./client "username"

        where username is one of the login to the server, which you registered before

        You have 3 attempts to type the password. After third wrong attempt, program will end.


6. How to use

    1. First make sure that server is running

    2. When you log in as a client you have 3 options what to do:

        When you press:
        - 1 - You will see all users which are actualy logged in
        - 2 - You can send message to yourself or actually logged users (For more information check 7.a)
        - 3 - You will logged out the user and quit the program

    3. To quit the server simply press q
        Server will log out all users and then quit
   

8. Features

    1. You can send message to more than one user. Simple just type the logins of users and divide them using space.
    
        For example:
        - Shell: Message send to(write username): 
        - User: login1 login2 login3 

    2. When user type a password to log in, you can not see how many characters his/her password has. This is for safety reasons.

    3. You can not register user with the same login and different password. Each user unique combination of login and password.

    4. You can not log in the same user to more than one console. Like real SMS system (You do not have 2 sim cards for one number)

    5. You can use only alphanumeric characters for username or password

    6. You can use all characters for messages except | (pipe)
    

9. How it works

    <img src="../schema.png"/>
        
 

