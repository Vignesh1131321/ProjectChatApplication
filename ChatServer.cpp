#include <jni.h>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <tuple>
#include <sstream>
#include <fstream>
#include <algorithm> // For transform
#include <cctype>    // For tolower

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define CLOSE_SOCKET(s) close(s)
#endif

#ifdef _WIN32
#include <direct.h> // For _mkdir
#include <iomanip>  // For
#else
#include <sys/stat.h> // For mkdir on Unix-based systems
#endif

using namespace std;

const string userHistoryDir = "./user_histories/";
const string groupHistoryDir = "./group_histories/";

static mutex fileMutex_;
static mutex roomMutex_;

class SocketBase
{
protected:
    void sendMessage(int socket, const string &message)
    {
        send(socket, message.c_str(), message.length(), 0);
    }
};

class ChatRoom          // Base class for chat rooms
{                           // Two classes PrivateChat and GroupChat inherit from this class    
protected:
    string roomName_;
    map<int, string> participants_;
    int adminSocket_;
    unordered_map<int, string> pendingRequests_;
    mutex room_Mutex_;

public:
    ChatRoom() : adminSocket_(-1) {}        

    unordered_map<int, string> getPendingRequests()
    {
        return pendingRequests_;
    }

    void addPendingRequest(int clientSocket, const string &username)
    {
        lock_guard<mutex> lock(roomMutex_);
        pendingRequests_[clientSocket] = username;
    }

    virtual ~ChatRoom() {}

    virtual void addParticipant(int socket, const string &username)
    {
        participants_[socket] = username;
    }

    virtual void removeParticipant(int socket)
    {
        participants_.erase(socket);
    }

    void setAdmin(int socket)
    {
        adminSocket_ = socket;
        return;
    }

    int getAdminSocket() const
    {
        return adminSocket_;
    }

    const map<int, string> &getParticipants() const
    {
        return participants_;
    }

    void processRequest(int requestSocket, bool approve, mutex &userMutex, mutex &roomMutex1_, const string &adminUsername, const string &groupName)
    {
        lock_guard<mutex> lock(room_Mutex_);

        auto it = pendingRequests_.find(requestSocket);
        if (it == pendingRequests_.end())
        {

            cerr << "Request not found for socket: " << requestSocket << endl;
            return;
        }
        string requesterName = it->second;
        pendingRequests_.erase(it);

        if (approve){
            const string userGroupsDir = "./user_groups/";
            #ifdef _WIN32
                _mkdir(userGroupsDir.c_str());
            #else
                mkdir(userGroupsDir.c_str(), 0777);
            #endif
            // Add participant to the chat room
            addParticipant(requestSocket, requesterName);

            // Notify the requester
            string approvalMessage = "Admin " + adminUsername + " approved your request to join group " + groupName + ".\n";
            send(requestSocket, approvalMessage.c_str(), approvalMessage.length(), 0);
        }
        else{
            // Notify the requester
            string rejectionMessage = "Admin " + adminUsername + " denied your request to join group " + groupName + ".\n";
            send(requestSocket, rejectionMessage.c_str(), rejectionMessage.length(), 0);
        }
    }
};

class DatabaseManager
{
public:
    void createDirectories()
    {
        #ifdef _WIN32
                _mkdir(userHistoryDir.c_str());
                _mkdir(groupHistoryDir.c_str());
        #else
                mkdir(userHistoryDir.c_str(), 0777);
                mkdir(groupHistoryDir.c_str(), 0777);
        #endif
    }

    bool updateUserSocket(const string &username, int socket)
    {
        lock_guard<mutex> fileLock(fileMutex_);
        string tempFile = "user_sockets_temp.txt";
        ifstream inputFile("user_sockets.txt");
        ofstream outputFile(tempFile);

        if (!outputFile.is_open())
        {
            cerr << "Error: Unable to open temporary file for writing." << endl;
            return false;
        }

        bool userFound = false;
        if (inputFile.is_open())
        {
            string line;
            while (getline(inputFile, line))
            {
                size_t delimiterPos = line.find(' ');
                if (delimiterPos != string::npos)
                {
                    string existingUser = line.substr(0, delimiterPos);
                    if (existingUser == username)
                    {
                        outputFile << username << " " << socket << "\n";
                        userFound = true;
                    }
                    else
                    {
                        outputFile << line << "\n";
                    }
                }
            }
            inputFile.close();
        }

        if (!userFound)
        {
            outputFile << username << " " << socket << "\n";
        }

        outputFile.close();

        if (ifstream("user_sockets.txt").is_open())
        {
            if (remove("user_sockets.txt") != 0)
            {
                cerr << "Error removing user_sockets.txt: " << strerror(errno) << endl;
                return false;
            }
        }

        if (rename(tempFile.c_str(), "user_sockets.txt") != 0)
        {
            cerr << "Error renaming temporary file: " << strerror(errno) << endl;
            return false;
        }

        return true;
    }

    int getSocket(const string &username)
    {
        ifstream inFile("user_sockets.txt");
        if (!inFile.is_open())
        {
            cerr << "Error opening user_sockets.txt for reading." << endl;
            return -1;
        }

        string line;
        while (getline(inFile, line))
        {
            stringstream ss(line);
            string user;
            int socket;
            if (getline(ss, user, ',') && ss >> socket)
            {
                if (user == username)
                {
                    return socket;
                }
            }
        }
        return -1; // Return -1 if the user is not found
    }

    void updateChatHistory(const string &sender, const string &recipient, const string &message)
    {
        string firstUser = min(sender, recipient);
        string secondUser = max(sender, recipient);
        string chatFile = userHistoryDir + firstUser + "_" + secondUser + ".txt";
        // Get the current time as a string
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        stringstream timeStream;
        timeStream << put_time(&tm, "%Y-%m-%d %H:%M:%S");
        string currentTime = timeStream.str();

        // Format the message
        string formattedMessage = sender + " -> " + recipient + ":" + message + " " + currentTime;

        // Write the message to the chat file
        ofstream outFile(chatFile, ios::app);
        if (!outFile.is_open())
        {
            cerr << "Error opening file: " << chatFile << endl;
            return;
        }
        outFile << formattedMessage << endl;
        outFile.close();
    }

    void updateGroupHistory(const string &sender, const string &room, const string &message)
    {
        // Update group history by writing the message to a file
        ofstream outFile(groupHistoryDir + room + ".txt", ios::app);
        if (outFile.is_open())
        {
            outFile << message << endl;
            outFile.close();
        }
        else
        {
            cerr << "Error opening history file for group: " << room << endl;
        }
    }

    vector<string> getChatHistory(string username_, string otherUser, int clientSocket_)
    {
        // Check for both possible file names
        string historyFile1 = userHistoryDir + username_ + "_" + otherUser + ".txt";
        string historyFile2 = userHistoryDir + otherUser + "" + username_ + ".txt";

        string historyFile;

        vector<string> history;
        // Determine which file exists
        ifstream infile1(historyFile1);
        if (infile1.is_open())
        {
            historyFile = historyFile1;
            infile1.close();
        }
        else
        {
            ifstream infile2(historyFile2);
            if (infile2.is_open())
            {
                historyFile = historyFile2;
                infile2.close();
            }
            else
            {
                // No history file found
                string noHistoryMsg = "No previous chat history between " + username_ + " and " + otherUser + ".\n";
                send(clientSocket_, noHistoryMsg.c_str(), noHistoryMsg.length(), 0);
                cout << noHistoryMsg;
                return history;
            }
        }

        // Read and send the content of the found history file
        ifstream infile(historyFile);
        if (infile.is_open())
        {
            string line;
            while (getline(infile, line))
            {
                history.push_back(line);
            }
        }
        return history;
    }

    void retrieveAndSendHistory(string username_, const string &message, int clientSocket_)
    {
        size_t spacePos = message.find(' ', 8);
        if (spacePos == string::npos)
        {
            string errorMsg = "Invalid history request format.";
            send(clientSocket_, errorMsg.c_str(), errorMsg.size(), 0);
            return;
        }

        string otherUser = message.substr(9); // Extract the username
        string historyFile1 = userHistoryDir + username_ + "_" + otherUser + ".txt";
        string historyFile2 = userHistoryDir + otherUser + "" + username_ + ".txt";

        ifstream infile;

        // Open the correct history file if it exists
        if (ifstream(historyFile1))
        {
            infile.open(historyFile1);
        }
        else if (ifstream(historyFile2))
        {
            infile.open(historyFile2);
        }
        else
        {
            string noHistoryMsg = "No previous chat history with " + otherUser + ".";
            send(clientSocket_, noHistoryMsg.c_str(), noHistoryMsg.size(), 0);
            return;
        }
        // Read and send history line by line
        if (infile.is_open())
        {
            string line;
            while (getline(infile, line))
            {
                string message = line + "\n";
                send(clientSocket_, message.c_str(), message.size(), 0);
            }
            infile.close();
        }
        else
        {
            string errorMsg = "Failed to retrieve chat history.";
            send(clientSocket_, errorMsg.c_str(), errorMsg.size(), 0);
        }
    }

    void retrieveAndSendGroupHistory(map<string, unique_ptr<ChatRoom>> &chatRooms_, const string &groupName, int clientSocket)
    {
        if (chatRooms_.find(groupName) == chatRooms_.end())
        {
            string errorMsg = "Group not found: " + groupName;
            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
            cerr << errorMsg << endl;
            return;
        }
        ChatRoom &group = *chatRooms_[groupName];

        map<int, string> participantsList = group.getParticipants();
        if (participantsList.find(clientSocket) == participantsList.end())
        {
            string errorMsg = "Access denied: You are not a participant of group: " + groupName + ".\n";
            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
            cerr << errorMsg << endl;
            return;
        }

        string groupFile = groupHistoryDir + groupName + ".txt";
        cout << "Looking for group history file: " << groupFile << endl;

        ifstream infile(groupFile);
        if (!infile.is_open())
        {
            string errorMsg = "No history found for group: " + groupName + ".\n";
            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
            cerr << errorMsg << endl;
            return;
        }

        cout << "Group history file opened: " << groupFile << endl;
        string line;
        while (getline(infile, line))
        {
            send(clientSocket, line.c_str(), line.length(), 0);
            send(clientSocket, "\n", 1, 0);
        }
        infile.close();
        cout << "Group history sent successfully for group: " << groupName << endl;
    }
};


class PrivateChat : public ChatRoom
{
public:
    PrivateChat() : ChatRoom() {}

    void addParticipant(int socket, const string &username) override
    {
        if (participants_.size() >= 2)
        {
            cerr << "Private chat cannot have more than two participants." << endl;
            return;
        }
        ChatRoom::addParticipant(socket, username);
    }

    void sendMessageToParticipant(int senderSocket, const string &message)
    {
        for (const auto &participant : participants_)
        {
            int socket = participant.first;
            const string &username = participant.second;
            if (socket != senderSocket)
            {
                send(socket, message.c_str(), message.length(), 0);
            }
        }
    }
};

class GroupChat : public ChatRoom
{
private:
    vector<pair<int, string>> pendingRequests_;

public:
    GroupChat() : ChatRoom() {}

    void addPendingRequest(int socket, const string &username)
    {
        pendingRequests_.emplace_back(socket, username);
    }

    const vector<pair<int, string>> &getPendingRequests() const
    {
        return pendingRequests_;
    }

    void processRequest(int requestSocket, bool approve, mutex &userMutex, mutex &roomMutex, const string &username, const string &groupName)
    {
        lock_guard<mutex> lock(roomMutex);
        auto it = std::find_if(pendingRequests_.begin(), pendingRequests_.end(),
                               [requestSocket](const pair<int, string> &req)
                               { return req.first == requestSocket; });

        if (it != pendingRequests_.end())
        {
            if (approve)
            {
                addParticipant(it->first, it->second);
                cout << it->second << " has been added to group " << groupName << endl;
            }
            pendingRequests_.erase(it);
        }
    }

    void broadcast(const string &message, int senderSocket)     // Broadcast the message to all participants
    {
        lock_guard<mutex> lock(room_Mutex_);
        int checker = 0;
        for (const auto &participant : participants_)
        {
            if (senderSocket == participant.first)
            {
                checker = 1;
                break;
            }
        }
        if (checker)
        {
            for (const auto &participant : participants_)
            {
                if (senderSocket != participant.first)
                {
                    int socket = participant.first;
                    const string &username = participant.second;
                    send(socket, message.c_str(), message.length(), 0);
                }
            }
        }
        else
        {
            const string &errormsg = "You are not a participant of this group";
            send(senderSocket, errormsg.c_str(), errormsg.length(), 0);
        }
    }
};

class ClientHandler
{
private:
    int clientSocket_;
    string username_;
    DatabaseManager *databaseManager_;
    map<string, int> &userSockets_;
    map<string, unique_ptr<ChatRoom>> &chatRooms_;
    mutex &userMutex_;
    mutex &roomMutex_;
    mutex fileMutex_; // Mutex for file operations

public:
    ClientHandler(int clientSocket, map<string, int> &userSockets,
    map<string, unique_ptr<ChatRoom>> &chatRooms, mutex &userMutex, mutex &roomMutex): clientSocket_(clientSocket), userSockets_(userSockets), chatRooms_(chatRooms),userMutex_(userMutex), roomMutex_(roomMutex) {}

    void operator()()
    {
        if (registerUser())
        {
            interactWithClient();
        }
        disconnect();
    }

    bool registerUser()
    {
        char buffer[1024];
        int bytesReceived = recv(clientSocket_, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0)
        {
            cerr << "Failed to register user." << endl;
            return false;
        }
        buffer[bytesReceived] = '\0';
        username_ = string(buffer);

        // Lock for thread-safe operations on userSockets_
        {
            lock_guard<mutex> lock(userMutex_);
            userSockets_[username_] = clientSocket_; // Update the in-memory map
        }

        cout << username_ << " has connected." << endl;

        // Create an instance of DatabaseManager
        DatabaseManager dbManager;

        // Update the socket information in the database
        if (!dbManager.updateUserSocket(username_, clientSocket_))
        {
            cerr << "Error updating user socket in the database." << endl;
            return false;
        }

        return true;
    }

    void interactWithClient()
    {
        char buffer[4096];
        while (true)
        {
            int bytesReceived = recv(clientSocket_, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0)
            {
                break;
            }
            buffer[bytesReceived] = '\0';
            processMessage(string(buffer));
        }
    }

    void processMessage(const string &message)      // Process the received message
    {
        if (message.rfind("/msg", 0) == 0)
        {
            sendMessageToUser(message);
        }
        else if (message.rfind("/group", 0) == 0)
        {
            broadcastToGroup(message);
        }
        else if (message.rfind("/join", 0) == 0)
        {
            joinGroup(message);
        }
        else if (message.rfind("/admin", 0) == 0)
        {
            handleAdminResponse(message, username_, clientSocket_);
        }
        else if (message.rfind("/create", 0) == 0)
        {
            createGroup(message);
        }
        else if (message.rfind("/listgroups", 0) == 0)
        {
            listGroups();
        }
        else if (message.rfind("/history", 0) == 0)
        {
            databaseManager_->retrieveAndSendHistory(username_, message, clientSocket_);
        }
        else if (message.rfind("/grpHistory", 0) == 0)
        {
            for (auto &room : chatRooms_)
            {
                cout << "Group: " << room.first << endl;
            }
            string groupName = message.substr(12); // Extract the group name
            cout << "This line is just before function call" << endl;
            databaseManager_->retrieveAndSendGroupHistory(chatRooms_, groupName, clientSocket_);
        }
        else
        {
            cout << "Received unknown command from client: " << message << endl;
        }
    }

    void createGroup(const string &message)
    {
        string room = message.substr(8);    // Extract room name from the message
        lock_guard<mutex> lock(roomMutex_); // Lock the mutex to protect shared resources

        // Check if the room already exists
        if (chatRooms_.find(room) == chatRooms_.end())
        {
            // Create a new chat room if it doesn't exist
            chatRooms_[room] = make_unique<ChatRoom>(); // Assuming ChatRoom is a valid class

            // Set the admin and add the participant (client) to the room
            chatRooms_[room]->setAdmin(clientSocket_);
            chatRooms_[room]->addParticipant(clientSocket_, username_);

            // Send a confirmation message to the client
            sendMessage(clientSocket_, ("Group created successfully: " + room));
            cout << username_ << " created group: " << room << endl;
        }
        else
        {
            // If the group already exists, notify the client
            sendMessage(clientSocket_, ("Group already exists: " + room));
        }
    }

    void joinGroup(const string &message)       // Handle group join requests
    {
        string roomName = message.substr(6);
        lock_guard<mutex> lock(roomMutex_);
        if (chatRooms_.find(roomName) != chatRooms_.end())
        {
            ChatRoom &room = *(chatRooms_[roomName]);
            int adminSocket = room.getAdminSocket();
            if (adminSocket != -1 && adminSocket != clientSocket_)
            {
                string requestMsg = "/admin approve " + username_ + " " + roomName;
                sendMessage(adminSocket, requestMsg);
                room.addPendingRequest(clientSocket_, username_);
                string waitMsg = "Request sent to admin for approval.";
                sendMessage(clientSocket_, waitMsg);
            }
            else
            {
                sendMessage(clientSocket_, "You are already the admin of this group.");
            }
        }
        else
        {
            string msg1 = "Group not found: " + roomName;
            sendMessage(clientSocket_, msg1);
        }
    }

    void handleAdminResponse(const string &message, const string &username, int clientSocket)   // Handle admin responses to join requests
    {
        size_t spacePos = message.find(' ', 7);
        string groupName = message.substr(7, spacePos - 7);
        string decision = message.substr(spacePos + 1);
        lock_guard<mutex> lock(roomMutex_);
        if (chatRooms_.find(groupName) != chatRooms_.end())
        {
            ChatRoom &room = *(chatRooms_[groupName]);
            if (clientSocket == room.getAdminSocket())
            {
                for (auto &request : room.getPendingRequests())
                {
                    int requestSocket = request.first;
                    string requesterName = request.second;
                    if (decision == "accept")
                    {
                        cout << 1 << endl;
                        room.processRequest(requestSocket, true, userMutex_, roomMutex_, username_, groupName);
                        string successMsg = "Admin approved your request to join " + groupName;
                        sendMessage(requestSocket, successMsg);
                    }
                    else if (decision == "reject")
                    {
                        room.processRequest(requestSocket, false, userMutex_, roomMutex_, username_, groupName);
                        string rejectionMsg = "Admin rejected your request to join " + groupName;
                        sendMessage(requestSocket, rejectionMsg);
                    }
                }
            }
            else
            {
                sendMessage(clientSocket, "You are not the admin of this group.");
            }
        }
        else
        {
            sendMessage(clientSocket, "Group not found.");
        }
    }

    void listGroups()
    {
        stringstream groupList;
        groupList << "Available groups: ";
        lock_guard<mutex> lock(roomMutex_);
        for (const auto &room : chatRooms_)
        {
            groupList << room.first << ", ";
        }
        string groups = groupList.str();
        if (!groups.empty())
        {
            groups.pop_back();
            groups.pop_back();
        }
        sendMessage(clientSocket_, groups);
    }

    void disconnect()
    {
        lock_guard<mutex> lock(userMutex_);
        CLOSE_SOCKET(clientSocket_);
        cout << username_ << " disconnected." << endl;
    }

    void handlePrivateMessage(const string &receiver, const string &message)
    {
        lock_guard<mutex> lock(userMutex_);

        auto it = userSockets_.find(receiver);
        if (it != userSockets_.end())
        {
            int receiverSocket = it->second;
            string fullMessage = username_ + ": " + message;
            send(receiverSocket, fullMessage.c_str(), fullMessage.length(), 0);
            databaseManager_->updateChatHistory(username_, receiver, message);
        }
        else
        {
            sendMessage(clientSocket_, "User not found or offline.");
        }
    }

    void sendMessage(int socket, const string &message)
    {

        send(socket, message.c_str(), message.length(), 0);
    }

    void sendMessageToUser(const string &message)
    {
        size_t firstSpace = message.find(' ', 5);
        if (firstSpace == string::npos)
            return;

        string recipient = message.substr(5, firstSpace - 5);
        string msg = message.substr(firstSpace + 1);
        lock_guard<mutex> lock(userMutex_);
        if (userSockets_.find(recipient) != userSockets_.end())
        {
            int recipientSocket = userSockets_[recipient];

            databaseManager_->updateChatHistory(username_, recipient, msg);
            sendMessage(recipientSocket, msg);
            cout << username_ << " sent to " << recipient << ": " << msg << endl;
        }
        else
        {
            string errorMsg = "User " + recipient + " not found.";
            sendMessage(clientSocket_, errorMsg);
        }
    }

    void broadcastToGroup(const string &message)
    {
        size_t firstSpace = message.find(' ', 7);
        // If the message does not contain a space after "groupName", return early
        if (firstSpace == string::npos)
            return;

        // Extract the room (group name) and the actual message
        string room = message.substr(7, firstSpace - 7);
        string msg = message.substr(firstSpace + 1);

        lock_guard<mutex> lock(roomMutex_); // Lock the mutex to ensure thread-safety while accessing chatRooms_

        // Check if the group exists
        auto it = chatRooms_.find(room);
        if (it != chatRooms_.end())
        {
            cout << "Broadcasting message to group: " << room << endl;

            try
            {
                // Check if the room is actually a GroupChat instance
                GroupChat *groupChat = reinterpret_cast<GroupChat *>(it->second.get());
                if (groupChat)
                {
                    // Successfully cast to GroupChat, now broadcast the message
                    groupChat->broadcast(msg, clientSocket_);
                    databaseManager_->updateGroupHistory(username_, room, msg);
                }
                else
                {
                    cerr << "Error: Group " << room << " is not a valid GroupChat instance." << endl;
                    sendMessage(clientSocket_, "Error: This is not a GroupChat.");
                    return;
                }
            }
            catch (const exception &e)
            {
                cerr << "Exception occurred while broadcasting message: " << e.what() << endl;
            }
        }
        else
        {
            // Group does not exist, send an error message back to the client
            string errorMsg = "Group " + room + " does not exist.";
            sendMessage(clientSocket_, errorMsg);
            cout << "Group " << room << " does not exist." << endl;
        }
    }
};

class ChatServer : public SocketBase
{
public:
    ChatServer(int port) : port_(port), listenSocket_(-1)
    {
#ifdef _WIN32
        WSAStartup(MAKEWORD(2, 2), &wsaData_);
#endif
    }

    ~ChatServer()
    {
        stopServer();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void start()
    {
        if (!setupServerSocket())
            return;
        cout << "Server started listening on port: " << port_ << endl;

        while (true)
        {
            int clientSocket = accept(listenSocket_, nullptr, nullptr);
            if (clientSocket == -1)
            {
                cerr << "Invalid client socket" << endl;
                continue;
            }

            activeThreads_.emplace_back(
                new std::thread([this, clientSocket]()
                                { 
                ClientHandler handler(clientSocket, userSockets_, chatRooms_, userMutex_, roomMutex_);
                handler(); }));
            activeThreads_.back()->detach();
        }
    }

private:
    int port_;
    int listenSocket_;
    map<string, int> userSockets_;
    map<string, unique_ptr<ChatRoom>> chatRooms_;
    vector<unique_ptr<thread>> activeThreads_;
    mutex userMutex_;
    mutex roomMutex_;

#ifdef _WIN32
    WSADATA wsaData_;
#endif

    bool setupServerSocket()                // Create a socket and bind it to the specified port
    {
        listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket_ == -1)
        {
            cerr << "Could not create socket." << endl;
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port_);

        if (bind(listenSocket_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            cerr << "Bind failed." << endl;
            CLOSE_SOCKET(listenSocket_);
            return false;
        }

        listen(listenSocket_, 5);
        return true;
    }

    void stopServer()           // Close the server socket and join all active threads
    {
        CLOSE_SOCKET(listenSocket_);
        for (auto &thread : activeThreads_)
        {
            if (thread->joinable())
            {
                thread->join();
            }
        }
    }
};

extern "C"
{
    JNIEXPORT void JNICALL Java_ChatApplication_startServer(JNIEnv *env, jobject obj, jint port)
    {
        ChatServer server(port);
        server.start();
    }
}