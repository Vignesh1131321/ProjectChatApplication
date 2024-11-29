#include <iostream>
#include <string>
#include <thread>
#include <cstring>                 // For memset
#include <mutex>
#include <queue>
#include <condition_variable>
#include <jni.h>                   // For JNI
#include <sstream> 
#include <direct.h> // For _mkdir  
#include <fstream>        
#include <algorithm> // For transform
#include <cctype>    // For tolower


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>     // For POSIX close
#include <arpa/inet.h>  // For POSIX sockets on Linux/macOS
#include <sys/socket.h> // For socket functions
#endif

using namespace std;

mutex inputMutex, queueMutex;
condition_variable adminCondition;
queue<string> adminRequests;
const string userHistoryDir = "./user_histories/";
const string groupHistoryDir = "./group_histories/";

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


void sendHistoryRequest(int s,const string &command)
{
    send(s, command.c_str(), command.length(), 0);
    cout << "Requested chat history from the server..." << endl;
}

void retrieveGroupHistory(string groupName, int clientSocket_)
{
    // Normalize group name to lowercase for consistency
    transform(groupName.begin(), groupName.end(), groupName.begin(), ::tolower);

    // Construct the file path for the group chat history
    string groupFile = groupHistoryDir + groupName + ".txt";

    // Attempt to open the group chat history file
    ifstream infile(groupFile);
    if (!infile.is_open())
    {
        // Debug: File not found
        cerr << "Group chat history file not found: " << groupFile << endl;

        // Inform the client
        string noHistoryMsg = "Error: Group '" + groupName + "' does not exist.\n";
        send(clientSocket_, noHistoryMsg.c_str(), noHistoryMsg.length(), 0);
        throw runtime_error(noHistoryMsg); // Throw an exception to handle invalid groups upstream
    }

    // Debug: File found
    cout << "Group chat history file found: " << groupFile << endl;

    // Read and send the content of the group chat history file
    string line;
    while (getline(infile, line))
    {
        cout << line << endl; 
    }
    infile.close();
}

// Function to initialize the sockets library for Windows
void InitializeSockets()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed" << endl;
        exit(1);
    }
#endif
}

// Function to clean up sockets on Windows
void CleanupSockets()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

// Cross-platform function to close a socket
void CloseSocket(int s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

// Function to handle receiving messages from the server
void ReceiveMessages(int s)
{
    char buffer[4096];
    while (true)
    {
        int recvLength = recv(s, buffer, sizeof(buffer), 0);
        if (recvLength > 0)
        {
            string receivedMsg(buffer, recvLength);
            cout << "Received: " << receivedMsg << endl;
            if (receivedMsg.find("/admin") == 0)
                {
                    lock_guard<mutex> lock(queueMutex);
                    adminRequests.push(receivedMsg);

                    cout << "Admin request queued: " << receivedMsg << endl;
                }
        }
        else
        {
            // If recvLength <= 0, either the server closed the connection or an error occurred
            cout << "Connection closed or error occurred" << endl;
            break;
        }
    }
}

// Combined function to handle both sending and receiving messages
void HandleChat(int s, const string &name)
{
    // Start a thread for receiving messages
    std::thread receiverThread(ReceiveMessages, s);
    receiverThread.detach(); // Detach the thread to allow it to run independently

    char buffer[4096];
    string message;

    // Send the username to the server
    send(s, name.c_str(), name.length(), 0);

    while (true)
    {
        // User input handling
        cout << "Choose action: 1. Private message, 2. Join group, 3. Send group message, 4. Create group, 5. Admin Requests, 6. Quit" << endl;
        int choice;
        cin >> choice;
        cin.ignore(); // Clear the input buffer

        if (choice == 6)
        {
            cout << "Stopping the chat" << endl;
            break;
        }

        switch (choice)
        {
        case 1:
        {
            cout << "Enter recipient's username: ";
            string recipient;
            cin >> recipient;
            sendHistoryRequest(s,"/history " + recipient);
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string msg1;
            cout << "Enter your message: ";
            // cin>>msg1;
            getline(cin, msg1);
            string msg = "/msg " + recipient + " " + name + " : " + msg1;
            if (send(s, msg.c_str(), msg.length(), 0) == -1)
            {
                cerr << "Failed to send message" << endl;
            }
            break;
        }
        case 2:
        {
            cout << "Enter group name to join: ";
            string group;
            cin >> group;

            string msg = "/join " + group;
            if (send(s, msg.c_str(), msg.length(), 0) == -1)
            {
                cerr << "Failed to join group" << endl;
            }
            
            break;
        }
        case 3:
        {
            cout << "Enter group name: ";
            string group;
            cin >> group;

            sendHistoryRequest(s,"/grpHistory " + group);

            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear input buffer

            cout << "Enter your group message: ";
            getline(cin, message);

            string msg = "/group " + group + " " + name + " : " + message;
            if (send(s, msg.c_str(), msg.length(), 0) == -1)
            {
                cerr << "Failed to send group message" << endl;
            }
            break;
        }
        case 4:
        {
            cout << "Create a new group" << endl;
            string groupname;
            cin >> groupname;
            cout << " Group name: " << groupname << endl;
            string msg = "/create " + groupname;
            if (send(s, msg.c_str(), msg.length(), 0) == -1)
            {
                cerr << "Failed to create group" << endl;
            }
            break;
        }
        case 5:
        {
            cout << "Processing admin requests..." << endl;
            while (true){
                cout << "\nAdmin Requests in Queue:" << endl;
                int index = 1;
                queue<string> tempQueue = adminRequests; // Temporary queue to display requests
                while (!tempQueue.empty())
                {
                    cout << index++ << ". " << tempQueue.front() << endl;
                    tempQueue.pop();
                }

                cout << "Choose a request to process (or enter 0 to skip): ";
                int choice;
                cin >> choice;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                if (choice > 0 && choice <= static_cast<int>(adminRequests.size()))
                {
                    // Extract the chosen request
                    queue<string> newQueue;
                    int currentIndex = 1;
                    string chosenRequest;

                    while (!adminRequests.empty())
                    {
                        if (currentIndex == choice)
                        {
                            chosenRequest = adminRequests.front();
                        }
                        else
                        {
                            newQueue.push(adminRequests.front());
                        }
                        adminRequests.pop();
                        currentIndex++;
                    }

                    adminRequests = newQueue;

                    // Handle the chosen request
                    cout << "Processing admin request: " << chosenRequest << endl;

                    if (chosenRequest.find("/admin approve") == 0)
                    {
                        size_t firstSpace = chosenRequest.find(' ', 15);
                        std::string userN = chosenRequest.substr(15, firstSpace - 15);
                        string groupToApprove = chosenRequest.substr(firstSpace + 1);
                        cout << "Approve group " << groupToApprove << " (y/n)? ";
                        char response;
                        cin >> response;
                        if (response == 'y' || response == 'Y')
                        {
                            string approveMsg = "/admin " + groupToApprove + " accept";
                            send(s, approveMsg.c_str(), approveMsg.length(), 0);
                            cout << "Group " << groupToApprove << " approved." << endl;
                        }
                    }
                }
                else if (choice == 0)
                {
                    cout << "No action taken for admin requests." << endl;
                    break;
                }
            }
            break;
        }
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    }

    CloseSocket(s);        // Close the socket after the user quits
    receiverThread.join(); // Ensure the receiver thread finishes before exiting
}

// JNI method implementation
extern "C"
{
    JNIEXPORT void JNICALL Java_ChatApplication_connectToServer(JNIEnv *env, jobject obj, jstring username, jstring serverAddress, jint port)
    {
        // Convert jstring to std::string
        const char *serverAddressCStr = env->GetStringUTFChars(serverAddress, nullptr);
        const char *usernameCStr = env->GetStringUTFChars(username, nullptr);

        // Initialize sockets
        InitializeSockets();

        // Create a TCP socket
        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1)
        {
            cerr << "Socket creation failed" << endl;
            CleanupSockets();
            return;
        }

        // Setup server address
        sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr)); // Zero out the struct
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port); // Convert port to network byte order

        // Convert the server address from text to binary form
        if (inet_pton(AF_INET, serverAddressCStr, &(serveraddr.sin_addr)) <= 0)
        {
            cerr << "Invalid address or address not supported" << endl;
            CloseSocket(s);
            CleanupSockets();
            return;
        }

        // Connect to the server
        if (connect(s, reinterpret_cast<sockaddr *>(&serveraddr), sizeof(serveraddr)) == -1)
        {
            cerr << "Connection to the server failed" << endl;
            CloseSocket(s);
            CleanupSockets();
            return;
        }

        // Handle chat messages (both sending and receiving)
        HandleChat(s, usernameCStr);

        // Cleanup
        CleanupSockets();

        // Release the jstring resources
        env->ReleaseStringUTFChars(serverAddress, serverAddressCStr);
        env->ReleaseStringUTFChars(username, usernameCStr);
    }
}
