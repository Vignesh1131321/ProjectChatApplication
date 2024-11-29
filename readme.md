# Chat Application

## Overview

This project is a chat application that includes functionalities for user authentication, group chat management, and message history retrieval. The project is implemented using a combination of Java and C++.

## Features

- **User Authentication**: Users can register and log in using their username and password.
- **Private Chat Management**: Users can initiate private chats and send private messages to other users.
- **Group Chat Management**: Users can create, join, and broadcast messages to groups. Group creators can assign
- **Message History**: Chat history is stored and can be retrieved for both individual and group chats.

## Project Structure

    .vscode/
        c_cpp_properties.json
        launch.json
        settings.json
    AuthServer/

        AuthServer.java
        database.txt
    AuthUser/
        AuthUser.java
    ChatApplication.h
    ChatApplication.java
    ChatClient.dll
    ChatClient.h
    ChatClient.java
    ChatServer.cpp
    ChatServerConnector.dll
    ChatSession.java
    Client.cpp
    group_histories/
    readme.md
    user_groups/
    user_histories/
    user_sockets.txt

Instructions to Run the Program:

Step 1:

Compile the C++ Files and Create DLL
Open a terminal and navigate to the temp1 directory.

Compile the C++ files and create the DLL file using the following commands:

    g++ -I"C:\Path\To\Your\JDK\include" -I"C:\Path\To\Your\JDK\include\win32" -shared -o ChatServerConnector.dll ChatServer.cpp -lws2_32 -pthread

    g++ -I"C:\Path\To\Your\JDK\include" -I"C:\Path\To\Your\JDK\include\win32" -shared -o ChatClient.dll Client.cpp -lws2_32 -pthread

Step 2:

Run the Authentication Server
Open a new terminal.

Navigate to the AuthServer directory.

Run the AuthServer.java file:

    javac AuthServer.java
    java AuthServer

Step 3: 

Run the Chat Server
Open another terminal.

Navigate to the source directory.

Run the ChatApplication.java file:

    javac ChatApplication.java
    java ChatApplication

Step 4:

Run the Chat Client
Open another terminal.

Navigate to the source directory.

Run the ChatClient.java file:

    javac ChatClient.java
    java ChatClient

Functionality

User Authentication

Register: Users can register by providing a username, phone number, and password.
Login: Users can log in using their username and password.

Private Chat Management

Start Private Chat: Users can initiate a private chat with another user.
Send Private Message: Users can send private messages to another user.

Group Chat Managements

Create Group: Users can create a new group.
Join Group: Users can join an existing group.
Broadcast Message: Users can send messages to all members of a group.
Admin Management: Group creator will be the admin, allowing them to manage group settings and members.

Message History

Store Chat History: Messages are stored in text files for both individual and group chats.
Retrieve Chat History: Users can retrieve the chat history for both individual and group chats.
    

Conclusion

This project provides a comprehensive chat application with user authentication, group chat management, and message history functionalities. The combination of Java and C++ allows for efficient handling of client-server interactions and data management. The project is well-structured, with clear separation of concerns across different components.

