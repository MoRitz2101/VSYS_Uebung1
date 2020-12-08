
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
#include <ctype.h>
namespace fs = std::filesystem;
#define BUF 1024
using namespace std;

vector<string> split_string(const string &str, const string &delimiter);
string convertToString(char *a, int size);
string sendMessage(vector<string> fromClient, string pathFromTerminal);
string listMessages(string user, string pathFromTerminal);
string deleteMessage(string user, string uuid, string pathFromTerminal);
string readMessage(string user, string uuid, string pathFromTerminal);
string getPathfromUUID(string path, string uuidWanted);
string get_uuid();
string encrypt(string text);
string decrypt(string text);
mutex mtx;

void *connectionHandler(int clientSocket, sockaddr_in client, string pathFromTerminal);

/**
 * Starting point of the Server
 * Validates Users arguments 
 * Starts listening Socket
 * Upon connecting with a Client, starts Thread to handle Client and continues listening
 */
int main(int argc, char *argv[])
{
    // Validate amount of Parameters
    if (argc < 3)
    {
        cerr << "Please enter Port Number and Directory Name" << endl;
        return -1;
    }
    //Validate Port Number
    if (atoi(argv[1]) < 1024 || atoi(argv[1]) > 49151)
    {
        cerr << "No valid Port Number" << endl;
        cerr << "Has to be in range 1024-49151" << endl;
        return -1;
    }
    int port = atoi(argv[1]);
    //Validate Directory Name
    if (!regex_match(argv[2], regex("^[^ \\/]*")))
    {
        cerr << "Invalid Directory Name" << endl;
        cerr << "Please enter the name of your storage Directory without the /" << endl;
        return -1;
    }
    string pathFromTerminal = argv[2];

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
    cerr << "Waiting for Connection...." << endl;
    while (true)
    {

        // Wait for a connection
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);

        //Create Client Socket by Accepting incoming request
        int clientSocket = accept(listening, (sockaddr *)&client, &clientSize);
        //Start new Thread using connection handler and by passing the client socket
        thread t(connectionHandler, clientSocket, client, pathFromTerminal);
        //detatch Thread so its not joinable
        t.detach();
    }
    return 0;
}

/**
 * Executed in a Thread
 * Handles Incoming Client Connections
 * Maps Clients Commands to correspondding Functions
 */
void *connectionHandler(int clientSocket, sockaddr_in client, string pathFromTerminal)
{
    char host[NI_MAXHOST];    // Client's remote name
    char service[NI_MAXSERV]; // Service (i.e. port) the client is connect on

    memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    if (getnameinfo((sockaddr *)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        cout << host << " connected " << endl;
        char welcome[50];
        strcpy(welcome, encrypt("Welcome to Server, Please enter your command:\n").c_str());
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
            cout << "Waiting for Connection...." << endl;
            break;
        }

        buf[bytesReceived] = '\0';

        string bufferAsString = convertToString(buf, bytesReceived);
        // cout<<bufferAsString<<endl;
        bufferAsString = decrypt(bufferAsString);
        // cout<<bufferAsString<<endl;
        //split Message to Vector by \n
        vector<string> splittedMessage = split_string(bufferAsString, "\n");
        string response = "failedTofindCommand";
        //clear BUffer
        buf[0] = 0;
        //Use mtx.lock/unlock to prevent Race Condition
        if (splittedMessage.at(0) == "SEND")
        {
            if (splittedMessage.size() > 5)
            {
                mtx.lock();
                response = sendMessage(splittedMessage, pathFromTerminal);
                splittedMessage.clear();
                mtx.unlock();
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
            mtx.lock();
            response = listMessages(splittedMessage.at(1), pathFromTerminal);
            mtx.unlock();
        }
        else if (splittedMessage.at(0) == "DEL")
        {
            mtx.lock();
            response = deleteMessage(splittedMessage.at(1), splittedMessage.at(2), pathFromTerminal);
            mtx.unlock();
        }
        response = response + "\0";
        response = encrypt(response);
        bytesReceived = response.size();
        send(clientSocket, response.c_str(), bytesReceived, 0);
    }

    // Close the socket
    close(clientSocket);

    return 0;
}

/**
 * Splits string at defined delimiter and returns the result as vector 
 */
vector<string> split_string(const string &str,
                            const string &delimiter)
{
    vector<string> strings;
    strings.clear();
    string::size_type pos = 0;
    string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != string::npos)
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    strings.push_back(str.substr(prev));

    return strings;
}

/**
 * Turns char array into String
 */
string convertToString(char *a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size; i++)
    {
        s = s + a[i];
    }
    return s;
}

/**
 * Implementation of SEND Command
 */
string sendMessage(vector<string> fromClient, string pathFromTerminal)
{
    string Sender = fromClient.at(1);
    string Empfaenger = fromClient.at(2);
    string Betreff = fromClient.at(3);

    //alle Zeilen in einen Nachrichten String speichern
    string Nachricht = "";
    for (int i = 4; i < (int)(fromClient.size() - 1); i++)
    {
        Nachricht = Nachricht + fromClient.at(i) + "\n";
    }
    string uuid = get_uuid();
    string folderPath = pathFromTerminal + "/" + Empfaenger;

    fromClient.clear();

    if (Sender.length() > 8 || Empfaenger.length() > 8 || Betreff.length() > 80)
    {
        return "failed\0";
    }
    //Folder erstellen wenn kein Folder besteht
    filesystem::create_directories(folderPath);

    string fullpath = folderPath + "/" + Betreff + "|" + uuid + ".txt";

    //Nachricht in Dokument schreiben
    ofstream fs(fullpath);

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

/**
 * Implementation of LIST Command
 */
string listMessages(string user, string pathFromTerminal)
{
    string path = "./" + pathFromTerminal + "/" + user;
    string directories = "";
    string mailList = "";
    string returnString = "";
    int counter = 0;
    //Teste ob der Pfad existiert
    DIR *dir = opendir(path.c_str());
    if (dir)
    {
        closedir(dir);
        //Alle Betreffs in einem String speichern und zurück geben
        for (const auto &entry : fs::directory_iterator(path))
        {
            directories = entry.path();
            int index = directories.find_last_of("/");
            string file = directories.substr(index + 1, directories.size());

            int fileindex = file.find_first_of("|");
            string fileName = file.substr(0, fileindex);
            int endindex = file.find_last_of(".");
            string uuid = file.substr(fileindex + 1, endindex - fileindex - 1);

            mailList = mailList + " Betreff: " + fileName + " UUID: " + uuid + "\n";

            counter++;
        }
        mailList = mailList + "\0";
        returnString = " Anzahl der Mails: " + to_string(counter) + "\n" + mailList + "\0";
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

/**
 * Implementation of DELETE Command
 */
string deleteMessage(string user, string uuid, string pathFromTerminal)
{
    string path = "./" + pathFromTerminal + "/" + user;

    string fullPath = getPathfromUUID(path, uuid);
    //trys to remove file
    if (fs::remove(fullPath))
    {
        return "ok\0";
    }
    else
    {
        return "failed\0";
    }
}

/**
 * Implementation of READ Command
 */
string readMessage(string user, string uuid, string pathFromTerminal)
{
    string path = "./" + pathFromTerminal + "/" + user;
    string fullPath = getPathfromUUID(path, uuid);
    string message = "";
    int counter = 0;
    ifstream file(fullPath);
    string str;
    //Alle Zeilen aus Dokument auslesen
    while (getline(file, str))
    {
        message = message + str + "\n";
        counter++;
    }
    if (counter > 0)
        return message;
    else
    {
        return "failed \n";
    }
}

string getPathfromUUID(string path, string uuidWanted)
{
    string directorie = "";
    DIR *dir = opendir(path.c_str());
    //Testen ob Pfad existiert
    if (dir)
    {
        closedir(dir);
        //Pfad bekommen wenn nur der User angegeben wird und der Pfad
        for (const auto &entry : fs::directory_iterator(path))
        {
            directorie = entry.path();
            int index = directorie.find_last_of("/");
            string file = directorie.substr(index + 1, directorie.size());
            int fileindex = file.find_first_of("|");
            string fileName = file.substr(0, fileindex);
            int endindex = file.find_last_of(".");
            string uuid = file.substr(fileindex + 1, endindex - fileindex - 1);

            if (uuid.compare(uuidWanted) == 0)
            {
                return directorie;
            }
        }
    }
    return string("error");
}
//UUID erstellen
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
string encrypt(string text)
{
    int s = 4;
    string result = "";
    //traverse text
    for (int i = 0; i <= (int)text.length(); i++)
    {
        // apply transformation to each character

        if (isupper(text[i]))
            result += char(int(text[i] + s - 65) % 26 + 65);

        // Encrypt Lowercase letters
        else if (islower(text[i]))
            result += char(int(text[i] + s - 97) % 26 + 97);

        else
            result += text[i];
    }
    //Return the resulting string
    return result;
}
string decrypt(string text)
{
    int s = 26 - 4;
    string result = "";
    //traverse text
    for (int i = 0; i <= (int)text.length(); i++)
    {
        // apply transformation to each character
        if (isupper(text[i]))
            result += char(int(text[i] + s - 65) % 26 + 65);

        // Encrypt Lowercase letters
        else if (islower(text[i]))
            result += char(int(text[i] + s - 97) % 26 + 97);

        else
            result += text[i];
    }
    //Return the resulting string
    return result;
}
