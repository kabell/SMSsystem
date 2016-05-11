1. Introduction
    This simple program you can use for sending messages between 2 or more clients.

2. Requirements
    - gcc - version supporting standard gnu99
    - make 
    - unix like operating system
    - write access


3. Install 
    git clone https://github.com/kabell/SMSsystem.git
    cd SMSsystem
    make

4. How to run
    
    4.1 - At first run server
        ./server
    
    4.2 - Then you can run how many clients you want
        ./client "username"
        where username is login to the server
    
5. Features
    
    5.1 - Server features
        
        5.1.1 - Add user to system
            ./server adduser "username"
            where username is login for user
            After that you will be asked 2 times for password for this user. If passwords are same
            user will be added to server. You can add users when server is running.

    5.2 - Client features
        


    
