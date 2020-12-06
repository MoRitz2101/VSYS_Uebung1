
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <bits/stdc++.h>
#include <filesystem>
#include <thread>
#include <mutex>
#include <regex>
namespace fs = std::filesystem;
#define BUF 1024
using namespace std;

std::vector<std::string> split_string(const std::string &str, const std::string &delimiter);
std::string convertToString(char *a, int size);
std::string sendMessage(std::vector<std::string> fromClient , std::string pathFromTerminal);
std::string listMessages(std::string user, std::string pathFromTerminal);
std::string deleteMessage(std::string user, std::string uuid, std::string pathFromTerminal);
std::string readMessage(std::string user, std::string uuid, std::string pathFromTerminal);
std::string getPathfromUUID(std::string path, std::string uuidWanted);
std::string get_uuid();
std::mutex mtx;
void *connectionHandler(int clientSocket, sockaddr_in client, std::string pathFromTerminal);

int main(int argc, char *argv[])
{
    if( argc < 3 ){
        cerr << "Please enter Port Number and Path" << endl;
        return -1;
    }
    if (atoi(argv[1]) < 1024 ||atoi(argv[1])>49151){
        cerr << "No valid Port Number" << endl;
        cerr << "Has to be in range 1024-49151" << endl;
        return -1;
    }
    int port = atoi(argv[1]);
    if (!std::regex_match(argv[2],std::regex("^[^ \\/]*"))){
        cerr << "Invalid Pathname" << endl;
        cerr << "Please enter the name of your storage Directory without the /" << endl;
        return -1;
    }
    std::string pathFromTerminal = argv[2];
    
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        cerr << "Can't create a socket! Quitting" << endl;
        return -1;
    }
    

    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    //Make sure listening port can be reused after being closed
    int flag = 1;
    if (-1 == setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        cerr << ("setsockopt fail") << endl;
    }

    bind(listening, (sockaddr *)&hint, sizeof(hint));
    // Tell Winsock the socket is for listening
    listen(listening, SOMAXCONN);

    while (true)    {
        cerr << "Waiting for Connection...." << endl;
        // Wait for a connection
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);

        //Create Client Socket by Accepting incoming request
        int clientSocket = accept(listening, (sockaddr *)&client, &clientSize);
        //Start new Thread using connection handler and by passing the client socket
        cout << pathFromTerminal << endl;
        std::thread t(connectionHandler, clientSocket, client, pathFromTerminal);
        t.detach();
        //close(listening);
    }

    cout << "ende" << endl;
    return 0;
}

void *connectionHandler(int clientSocket, sockaddr_in client, std::string pathFromTerminal)
{
    char host[NI_MAXHOST];    // Client's remote name
    char service[NI_MAXSERV]; // Service (i.e. port) the client is connect on

    memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    if (getnameinfo((sockaddr *)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        cout << host << " connected "<< endl;
        char welcome[50];
        strcpy(welcome, "Welcome to Server, Please enter your command:\n");
        send(clientSocket, welcome, strlen(welcome), 0);
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
         cout << host << " connected " << endl;
        char welcome[50];
        strcpy(welcome, "Welcome to Server, Please enter your command:\n");
        send(clientSocket, welcome, strlen(welcome), 0);
    }

    // Close listening socket
    //cout << "close Listening" << endl;
    //close(listening);

    // While loop: accept and echo message back to client
    char buf[4096];

    while (true)
    {
        memset(buf, 0, 4096);

        // Wait for client to send data
        int bytesReceived = recv(clientSocket, buf, 4095, 0);
        if (bytesReceived == -1)
        {
            cerr << "Error in recv(). Quitting" << endl;
            break;
        }

        if (bytesReceived == 0)
        {
            cout << "Client disconnected " << endl;
            break;
        }

        buf[bytesReceived] = '\0';

        std::string bufferAsString = convertToString(buf, bytesReceived);
        std::vector<std::string> splittedMessage = split_string(bufferAsString, "\n");
        std::string response = "failedTofindCommand";
        buf[0] = 0;
        if (splittedMessage.at(0) == "SEND")
        {
            if (splittedMessage.size() > 5)
            {
                response = sendMessage(splittedMessage, pathFromTerminal);
                splittedMessage.clear();
            }
            else
            {
                response = "failed\n";
            }
        }
        else if (splittedMessage.at(0) == "READ")
        {
            mtx.lock();
            response = readMessage(splittedMessage.at(1), splittedMessage.at(2), pathFromTerminal);
            mtx.unlock();
        }
        else if (splittedMessage.at(0) == "LIST")
        {
            mtx.unlock();
            response = listMessages(splittedMessage.at(1),pathFromTerminal);
            mtx.unlock();
        }
        else if (splittedMessage.at(0) == "DEL")
        {
            mtx.unlock();
            response = deleteMessage(splittedMessage.at(1), splittedMessage.at(2),pathFromTerminal);
            mtx.unlock();
        }
        response = response + "\0";
        bytesReceived = response.size();
        send(clientSocket, response.c_str(), bytesReceived, 0);
    }

    // Close the socket
    close(clientSocket);

    return 0;
}

std::vector<std::string> split_string(const std::string &str,
                                      const std::string &delimiter)
{
    std::vector<std::string> strings;
    strings.clear();
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos)
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    strings.push_back(str.substr(prev));

    return strings;
}

std::string convertToString(char *a, int size)
{
    int i;
    std::string s = "";
    for (i = 0; i < size; i++)
    {
        s = s + a[i];
    }
    return s;
}

std::string sendMessage(std::vector<std::string> fromClient , std::string pathFromTerminal)
{
    std::string Sender = fromClient.at(1);
    std::string Empfaenger = fromClient.at(2);
    std::string Betreff = fromClient.at(3);
    
    std::string Nachricht= "";
    for(int i = 4; i < (int)(fromClient.size()-1); i++){
        Nachricht = Nachricht + fromClient.at(i) + "\n";
    }
    std::string uuid = get_uuid();
    std::string folderPath = pathFromTerminal + "/" + Empfaenger;

    fromClient.clear();

    if (Sender.length() > 8 || Empfaenger.length() > 8 || Betreff.length() > 80)
    {
        return "failed\0";
    }
    std::filesystem::create_directories(folderPath);

      //  mkdir(folderPath.c_str(), 0777);

    std::string fullpath = folderPath + "/" + Betreff + "|" + uuid + ".txt";

    std::ofstream fs(fullpath);

    if (!fs)
    {
        return "failed\0";
    }
    fs << ("Sender: " + Sender + "\n").c_str();
    fs << ("Empfaenger: " + Empfaenger + "\n").c_str();
    fs << ("Betreff: " + Betreff + "\n").c_str();
    fs << ("Message: \n" + Nachricht).c_str();
    fs.close();
    return "ok\0";
}

std::string listMessages(std::string user , std::string pathFromTerminal)
{
    std::string path = "./" + pathFromTerminal + "/" + user;
    std::string directories = "";
    std::string mailList = "";
    std::string returnString = "";
    int counter = 0;

    DIR *dir = opendir(path.c_str());
    if (dir)
    {
        closedir(dir);
        for (const auto &entry : fs::directory_iterator(path))
        {
            directories = entry.path();
            int index = directories.find_last_of("/");
            std::string file = directories.substr(index + 1, directories.size());

            int fileindex = file.find_first_of("|");
            std::string fileName = file.substr(0, fileindex);
            int endindex = file.find_last_of(".");
            std::string uuid = file.substr(fileindex + 1, endindex - fileindex - 1);

            mailList = mailList + " Betreff: " + fileName + " UUID: " + uuid + "\n";

            counter++;
        }
        mailList = mailList + "\0";
        returnString = " Anzahl der Mails: " + std::to_string(counter) + "\n" + mailList + "\0";
    }
    else if (ENOENT == errno)
    {
        returnString = "failed User not found\0";
    }
    else
    {
        returnString = "failed\0";
    }

    return returnString;
}

std::string deleteMessage(std::string user, std::string uuid, std::string pathFromTerminal)
{
    std::string path = "./" + pathFromTerminal + "/" + user;

    std::string fullPath = getPathfromUUID(path, uuid);
    if (fs::remove(fullPath))
    {
        return "ok\0";
    }
    else
    {
        return "failed\0";
    }
}

std::string readMessage(std::string user, std::string uuid, std::string pathFromTerminal)
{
    std::string path = "./" + pathFromTerminal + "/" + user;
    std::string fullPath = getPathfromUUID(path, uuid);
    std::string message = "";
    int counter = 0;
    std::ifstream file(fullPath);
    std::string str;
    while (std::getline(file, str))
    {
        message = message + str + "\n";
        counter ++;
    }
    if(counter>0)
    return message;
    else{
        return "failed \n";
    }
}

std::string getPathfromUUID(std::string path, std::string uuidWanted)
{
    std::string directorie = "";
    DIR *dir = opendir(path.c_str());
    if (dir)
    {
        closedir(dir);
        for (const auto &entry : fs::directory_iterator(path))
        {
            directorie = entry.path();
            int index = directorie.find_last_of("/");
            std::string file = directorie.substr(index + 1, directorie.size());
            int fileindex = file.find_first_of("|");
            std::string fileName = file.substr(0, fileindex);
            int endindex = file.find_last_of(".");
            std::string uuid = file.substr(fileindex + 1, endindex - fileindex - 1);

            if (uuid.compare(uuidWanted) == 0)
            {
                return directorie;
            }
        }
    }
    return std::string("error");
}
string get_uuid()
{
    static random_device dev;
    static mt19937 rng(dev());

    uniform_int_distribution<int> dist(0, 15);

    const char *v = "0123456789abcdef";
    const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

    string res;
    for (int i = 0; i < 16; i++)
    {
        if (dash[i])
            res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }
    return res;
}
