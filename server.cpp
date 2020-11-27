
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
namespace fs = std::filesystem;
#define BUF 1024
using namespace std;

std::vector<std::string> split_string(const std::string &str, const std::string &delimiter);
std::string convertToString(char *a, int size);
std::string sendMessage(std::vector<std::string> fromClient);
std::string listMessages(std::string user);
std::string deleteMessage(std::string user, std::string uuid);
std::string readMessage(std::string user, std::string uuid);
std::string getPathfromUUID(std::string path, std::string uuidWanted);
std::string get_uuid();

int main(int argc, char **argv)
{
      if( argc < 2 ){
      cerr << "Please enter Port" << endl;
    return -1;
  }

    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        cerr << "Can't create a socket! Quitting" << endl;
        return -1;
    }

    // Bind the ip address and port to a socket
    sockaddr_in hint;
    if(atoi(argv[1])== 0){
        cerr << "No valid Port Number" << endl;
        return -1;
    }
    int port = atoi(argv[1]);
        
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    bind(listening, (sockaddr *)&hint, sizeof(hint));

    // the socket is for listening
    listen(listening, SOMAXCONN);
    cerr << "Waiting for Connection...." << endl;
    // Wait for a connection
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    int clientSocket = accept(listening, (sockaddr *)&client, &clientSize);

    char host[NI_MAXHOST];    // Client's remote name
    char service[NI_MAXSERV]; // Service (i.e. port) the client is connect on

    memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    if (getnameinfo((sockaddr *)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        cout << host << " connected on port " << port << endl;
        char welcome[50];
        strcpy(welcome, "Welcome to Server, Please enter your command:\n");
        send(clientSocket, welcome, strlen(welcome), 0);
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        cout << host << " connected on port " << port << endl;
        char welcome[50];
        strcpy(welcome, "Welcome to Server, Please enter your command:\n");
        send(clientSocket, welcome, strlen(welcome), 0);
    }

    // Close listening socket
    close(listening);

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
        if (splittedMessage.at(0) == " SEND")
        {
            if (splittedMessage.size() > 5)
            {
                response = sendMessage(splittedMessage);
                splittedMessage.clear();
            }
            else
            {
                response = "failed\n";
            }
        }
        else if (splittedMessage.at(0) == " READ")
        {
            std::cout<<"Into Read"<<std::endl;
            response = readMessage(splittedMessage.at(1), splittedMessage.at(2));
        }
        else if (splittedMessage.at(0) == " LIST")
        {
            response = listMessages(splittedMessage.at(1));
        }
        else if (splittedMessage.at(0) == " DEL")
        {
            response = deleteMessage(splittedMessage.at(1), splittedMessage.at(2));
        }
        else if (splittedMessage.at(0) == " QUIT")
        {
            
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
    std::string s = " ";
    for (i = 0; i < size; i++)
    {
        s = s + a[i];
    }
    return s;
}

std::string sendMessage(std::vector<std::string> fromClient)
{
    std::string Sender = fromClient.at(1);
    std::string Empfaenger = fromClient.at(2);
    std::string Betreff = fromClient.at(3);
    std::string Nachricht = fromClient.at(4);
    std::string uuid = get_uuid();
    std::string folderPath = "./mail/" + Empfaenger;

    fromClient.clear();

    if (Sender.length() > 8 || Empfaenger.length() > 8 || Betreff.length() > 80)
    {
        return "failed\0";
    }
    mkdir(folderPath.c_str(), 0777);

    std::string fullpath = folderPath + "/" + Betreff + "|" + uuid + ".txt";

    std::ofstream fs(fullpath);

    if (!fs)
    {
        return "failed\0";
    }
    fs << ("Sender: " + Sender+"\n").c_str();
    fs << ("Empfaenger: "+Empfaenger+"\n").c_str();
    fs << ("Betreff: " + Betreff+"\n").c_str();
    fs << ("Message: "+Nachricht).c_str();
    fs.close();
    return "ok\0";
}

std::string listMessages(std::string user)
{
    std::string path = "./mail/" + user;
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

std::string deleteMessage(std::string user, std::string uuid)
{
    std::string path = "./mail/" + user;

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

std::string readMessage(std::string user, std::string uuid){
   std::string path = "./mail/" + user;
   std::string fullPath = getPathfromUUID(path, uuid);
   std::string message = "";

    std::ifstream file(fullPath);
    std::string str; 
    while (std::getline(file, str))
    {
       message = message + str +"\n";
    }
    return message;
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
