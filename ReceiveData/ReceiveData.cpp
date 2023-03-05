#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <string>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

namespace po = boost::program_options;
using namespace std;

struct Header {
    int msgType;
    int dataLength;
    string fileName = "";
};

string encrypt(string msg, string key);
string RandomString(int ch);

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    int iResult;
    addrinfo* result = NULL, hints;
    string inputPort;
    int iSendResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    string out;
    SOCKET ClientSocket = INVALID_SOCKET;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        ("port,p", po::value<string>(&inputPort)->default_value("27600"), "Set port to listen for connection.")
        ("out,o", po::value<string>(&out)->default_value("D:\\socket out\\"), "Set directory to store received file.")
        ;
    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
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

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "Accept connection failed with error: " << WSAGetLastError();
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

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
                while (length > 0) {
                    ZeroMemory(&data, sizeof(data));
                    iResult = recv(ClientSocket, data, min(DEFAULT_BUFLEN, length), 0);
                    if (iResult > 0) {
                        for (int i = 0; i < min(iResult, length); i++) {
                            display += data[i];
                        }
                        totalRecv += iResult;
                        length -= iResult;
                    }
                    else if (result < 0) {
                        cout << "Receive data failed with error: " << WSAGetLastError() << "\n";
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                }
                display = encrypt(display, key);
                cout << "Successfully received " << totalRecv << " bytes" << " out of " << headerRecv.dataLength 
                    << " bytes of text (" << ((float)totalRecv * 100) / (float)headerRecv.dataLength
                    << "%) - Text received: \"" << display << "\"\n";
            }
            else if (msgType == 2){
                send(ClientSocket, pKey, 24, 0);
                ofstream file;
                char data[DEFAULT_BUFLEN];
                file.open(out + headerRecv.fileName, ios::binary | ios::trunc);
                string received = "";

                while (length > 0) {
                    ZeroMemory(&data, sizeof(data));
                    iResult = recv(ClientSocket, data, min(DEFAULT_BUFLEN, length), 0);
                    if (iResult > 0) {
                        string eString = encrypt(data, key);
                        file.write(eString.c_str(), iResult);
                        totalRecv += iResult;
                        length -= iResult;
                    }
                    else if (result < 0) {
                        cout << "Receive data failed with error: " << WSAGetLastError() << "\n";
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                }

                cout << "Successfully received " << totalRecv << " bytes" << " out of " << headerRecv.dataLength
                    << " bytes of data (" << ((float)totalRecv * 100) / (float)headerRecv.dataLength <<"%) - file received: \""  << out + headerRecv.fileName << "\"\n";
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

string encrypt(string msg, string key)
{
    string tmp(key);
    while (key.size() < msg.size())
        key += tmp;

    //encryption part
    for (string::size_type i = 0; i < msg.size(); ++i)
        msg[i] ^= key[i];
    return msg;
}