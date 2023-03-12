#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 8192

//namespace po = boost::program_options;
using namespace std;

struct Header {
    int msgType;
    int dataLength;
    int buffSize = 8192;
    string fileName = "";
};

void encrypt(char* msg, int size, const char* key, int sizeKey);
void encryptFile(char* msg, int size, const char* key, int sizeKey, int& j);
string RandomString(int ch);

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    int iResult;
    addrinfo* result = NULL, hints;
    string inputPort = "27600";
    int iSendResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    string out = "D:\\socket out\\";
    SOCKET ClientSocket = INVALID_SOCKET;
    string desc = "Allowed options:\n        - h[--help]                      produce help message.\n        - p[--port] arg(= 27600)         Set port to listen for connection.\n        - o[--out] arg(= D:\socket out\) Set directory to store received file.";

    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            if (string(argv[i]) == "-h" || string(argv[i]) == "--help") {
                cout << desc;
                return 1;
            }

            if (string(argv[i]) == "-p" || string(argv[i]) == "--port") {
                inputPort = string(argv[i + 1]);
            }

            if (string(argv[i]) == "-o" || string(argv[i]) == "--out") {
                out = string(argv[i + 1]);

                DWORD ftyp = GetFileAttributesA(out.c_str());
                if (ftyp == INVALID_FILE_ATTRIBUTES){
                    cout << "Your folder don't exist";
                    return 1;
                }

            }
        }
    }

    const char* PORT = inputPort.c_str();
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSA Startup failed with error: " << WSAGetLastError();
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    if ((getaddrinfo(NULL, PORT, &hints, &result) != 0)) {
        cout << "Get address infomation failed with error: " << WSAGetLastError();
        WSACleanup();
        return 1;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        cout << "Set listen socket failes with error: " << WSAGetLastError();
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        cout << "Bind failed with error: " << WSAGetLastError();
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed with error: " << WSAGetLastError();
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    cout << "\nListening for connection on port " << inputPort << "...";

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "Accept connection failed with error: " << WSAGetLastError();
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    SOCKADDR_IN addr;
    char ipinput[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ipinput, INET_ADDRSTRLEN);
    cout << "\rAccepted Connection from :  " << ipinput << "\n";

    Header headerRecv;
    while (1){
        int hResult = recv(ClientSocket, (char*)&headerRecv, (int)sizeof(headerRecv), 0);
        if (hResult > 0) {
            int msgType = (int)headerRecv.msgType;
            int length = headerRecv.dataLength;

            string key = RandomString(24);
            const char* pKey = key.c_str();
            int totalRecv = 0;
            if (msgType == 1){
                send(ClientSocket, pKey, 24, 0);
                string display = "";
                char data[DEFAULT_BUFLEN];

                cout << fixed << "Received: " << 0 << "%";
                while (length > 0) {
                    ZeroMemory(&data, sizeof(data));
                    iResult = recv(ClientSocket, data, min(headerRecv.buffSize, length), 0);
                    if (iResult > 0) {
                        for (int i = 0; i < min(iResult, length); i++) {
                            display += data[i];
                        }
                        totalRecv += iResult;
                        cout << "\rReceived: " << setprecision(2) << (totalRecv * 100.0) / headerRecv.dataLength << "%";
                        length -= iResult;
                    }
                    else if (result < 0) {
                        cout << "Receive data failed with error: " << WSAGetLastError() << "\n";
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                }
                encrypt((char*) display.data(), display.size(), key.data(), 24);
                cout << "\rSuccessfully received " << totalRecv << " bytes" << " out of " << headerRecv.dataLength 
                    << " bytes of text (" << ((float)totalRecv * 100) / (float)headerRecv.dataLength
                    << "%) "<< "from " << ipinput << "\nText received: \"" << display << "\"\n";
            }
            else if (msgType == 2){
                send(ClientSocket, pKey, 24, 0);
                ofstream file;
                file.open(out + headerRecv.fileName, ios::binary | ios::trunc);
                char data[8192];

                int start = 0;
                cout << fixed << "Received: " << 0 << "%";
                while (length > 0) {
                    ZeroMemory(&data, sizeof(data));
                    iResult = recv(ClientSocket, data, min(headerRecv.buffSize, length), 0);
                    if (iResult > 0) {
                        encryptFile(data, iResult, key.data(), 24, start);
                        file.write(data, iResult);
                        totalRecv += iResult;
                        cout << "\rReceived: " << setprecision(2) << (totalRecv * 100.0) / headerRecv.dataLength << "%";
                        length -= iResult;
                    }
                    else if (iResult < 0) {
                        cout << "\nReceive data failed with error: " << WSAGetLastError() << "\n";
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                }

                cout << "\rSuccessfully received " << totalRecv << " bytes" << " out of " << headerRecv.dataLength
                    << " bytes of data (" << ((float)totalRecv * 100) / (float)headerRecv.dataLength 
                    << "%) " << "from " << ipinput << "\nFile received : \"" << out + headerRecv.fileName << "\"\n";
            }
        }
        else {
            cout << "Receive header failed with error: " << WSAGetLastError() << "\n";
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
    }

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        cout << "shutdown failed with error: " << WSAGetLastError() << "\n";
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    WSACleanup();

    return 0;
}

string RandomString(int ch)
{
    char alpha[26] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                          'h', 'i', 'j', 'k', 'l', 'm', 'n',
                          'o', 'p', 'q', 'r', 's', 't', 'u',
                          'v', 'w', 'x', 'y', 'z' };
    string result = "";
    for (int i = 0; i < ch; i++)
        result = result + alpha[rand() % 26];

    return result;
}

void encrypt(char* msg, int size, const char* key, int sizeKey)
{
    int j = 0;

    //encryption part
    for (auto i = 0; i < size; ++i)
    {
        msg[i] ^= key[j];
        j = (j + 1) % sizeKey;
    }
    return;
}

void encryptFile(char* msg, int size, const char* key, int sizeKey, int& j)
{
    //encryption part
    for (auto i = 0; i < size; ++i)
    {
        msg[i] ^= key[j];
        j = (j + 1) % sizeKey;
    }
    return;
}