#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
    //	Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        return 1;
    }

    if (argc < 3)
    {
        cerr << "Please enter Port and Ip Adress" << endl;
        return -1;
    }
    
    //	Create a hint structure for the server we're connecting with
    cout<<atoi(argv[1])<<endl;
    if (atoi(argv[1]) == 0)
    {
        cerr << "No valid Port Number" << endl;
        return -1;
    }
    if (atoi(argv[1]) < 1024 ||atoi(argv[1])>49151){
        cerr << "No valid Port Number" << endl;
        cerr << "Has to be in range 1024-49151" << endl;
        return -1;
    }
    int port = atoi(argv[1]);
    string ipAddress = argv[2];

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    //	Connect to the server on the socket
    int connectRes = connect(sock, (sockaddr *)&hint, sizeof(hint));
    if (connectRes == -1)
    {
        cout << "There was an error getting response from server\r\n";
        return 0;
    }

    //	While loop:
    char buf[4096];
    string userInput;
    printf("Connection with server established\n");
    int bytesReceived = recv(sock, buf, 4096, 0);
    if (bytesReceived == -1)
    {
        cout << "There was an error getting response from server\r\n";
    }
    else
    {
        //		Display response
        cout << "SERVER> " << string(buf, bytesReceived) << "\r\n";
    }
    do
    {
        //		Enter lines of text
        cout << "Enter your command:> ";
        userInput = "";
        int x = 0;
        string line = "";
        while (getline(cin, line))
        {
            line = line + "\n";
            if (line.compare(".\n") != 0)
            {
                userInput = userInput + line;
            }
            else
            {
                x = 1;
                break;
            }
            if (x == 1)
                break;
        }

        //		Send to server
        if(userInput.compare("QUIT\n")==0)
            break;
        
        int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
        userInput = "";
        if (sendRes == -1)
        {
            cout << "Could not send to server! Whoops!\r\n";
            continue;
        }

        //		Wait for response
        memset(buf, 0, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);
        if (bytesReceived == -1)
        {
            cout << "There was an error getting response from server\r\n";
        }
        else
        {
            //		Display response
            cout << "SERVER> " << string(buf, bytesReceived) << "\r\n";
        }
    } while (true);

    //	Close the socket
    close(sock);

    return 0;
}