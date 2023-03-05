#include <winsock2.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <string>
#include <fstream>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 1024

namespace po = boost::program_options;
using namespace std;

struct Header {
    int msgType;
    int dataLength;
    string fileName;
};


void encrypt(char* msg, int size, const char* key, int sizeKey);
void encryptFile(char* msg, int size, const char* key, int sizeKey, int& j);
void sendtext(SOCKET ConnectSocket);
void sendfile(SOCKET ConnectSocket);

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    addrinfo* result = NULL, hints;
    string inputText;
    string inputAdd;
    string inputPort;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("des,d", po::value<string>(&inputAdd)->default_value("127.0.0.1"), "Set ip address to send data to")
        ("port,p", po::value<string>(&inputPort)->default_value("27600"), "Set port to sent data to")
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

    const char* IPADD = inputAdd.c_str();
    const char* PORT = inputPort.c_str();

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSA Startup failed with error: " << WSAGetLastError();
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(IPADD, PORT, &hints, &result)) {
        cout << "Get address infomation failed with error: " << WSAGetLastError();
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Set connect socket failed with error: " << WSAGetLastError();
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        cout << "Unable to connect to server!\n";
        closesocket(ConnectSocket);
        return 1;
    }

    int option = 1;
    while (option != 0){
        cout << "1 - Send text to receiver.\n";
        cout << "2 - Send file to receiver.\n";
        cout << "0 - Exit.\n";
        cout << "Please select option: ";
        cin >> option;
        switch (option){
        case 1:
            sendtext(ConnectSocket);
            break;
        case 2:
            sendfile(ConnectSocket);
            break;
        }

    }

    // shutdown the connection since no more data will be sent
    if (shutdown(ConnectSocket, SD_SEND) == SOCKET_ERROR) {
        cout << "Shutdown socket failed with error: " << WSAGetLastError() << "\n";
        closesocket(ConnectSocket);
        return 1;
    }

    WSACleanup();

    return 0;
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

void encryptFile(char* msg, int size, const char* key, int sizeKey, int &j)
{
    //encryption part
    for (auto i = 0; i < size; ++i)
    {
        msg[i] ^= key[j];
        j = (j + 1) % sizeKey;
    }
    return;
}

void sendtext(SOCKET ConnectSocket)
{
    int iResult;
    char key[24];
    string inputText;

    cout << "Plese enter your text: ";
    cin.ignore();
    getline(cin, inputText);

    Header headerSent;
    headerSent.msgType = 1;
    headerSent.dataLength = inputText.size();
    headerSent.fileName = "";

    iResult = send(ConnectSocket, (char*)&headerSent, (int)sizeof(headerSent), 0);
    if (iResult == SOCKET_ERROR) {
        cout << "Send header failed with error : " << WSAGetLastError() << "\n";
        closesocket(ConnectSocket);
        return;
    }

    ZeroMemory(&key, sizeof(key));
    if (recv(ConnectSocket, key, 24, 0)) {
        encrypt((char*) inputText.data(), inputText.size() + 1, key, 24);
        int totalSent = 0;

        int length = inputText.size() + 1;
        cout << fixed << "Sent: " << 0 << "%";
        while (length > 0) {
            iResult = send(ConnectSocket, (char*)inputText.data() + totalSent, min(length, DEFAULT_BUFLEN), 0);
            if (iResult == SOCKET_ERROR) {
                cout << "Send text failed with error : " << WSAGetLastError() << "\n";
                closesocket(ConnectSocket);
                return;
            }

            totalSent += iResult;
            cout << "\rSent: " << setprecision(2) << (totalSent * 100.0) / headerSent.dataLength << "%";
            length -= iResult;
        }
        cout << "\rSent " << setprecision(2) << 100.0 << "%)" << " bytes of data completed\n";
    }
    return;
}

void sendfile(SOCKET ConnectSocket)
{
    int iResult;
    char key[24];
    string filePath;
    char bufferFile[DEFAULT_BUFLEN];
    ifstream file;

    cout << "Plese enter your file path: ";
    cin >> filePath;
    file.open(filePath, ios::binary);
    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        Header headerSent;
        headerSent.msgType = 2;
        headerSent.dataLength = file.tellg();
        headerSent.fileName = filePath.substr(filePath.find_last_of("/\\") + 1);
        
        iResult = send(ConnectSocket, (char*)&headerSent, (int)sizeof(headerSent), 0);
        if (iResult == SOCKET_ERROR) {
            cout << "Send header failed with error : " << WSAGetLastError() << "\n";
            closesocket(ConnectSocket);
            return;
        }

        ZeroMemory(&key, sizeof(key));
        if (recv(ConnectSocket, key, 24, 0)) {
            file.seekg(0, ios::beg);

            int length = headerSent.dataLength;
            int start = 0;
            int totalSent = 0;
            cout << fixed << "Sent: 0%";
            while(length > 0){
                file.read(bufferFile, DEFAULT_BUFLEN);
                if (file.gcount() > 0) {
                    encryptFile(bufferFile, file.gcount(), key, 24, start);
                    iResult = send(ConnectSocket, bufferFile, file.gcount(), 0);
                    if (iResult == SOCKET_ERROR) {
                        cout << "Send file failed with error : " << WSAGetLastError() << "\n";
                        closesocket(ConnectSocket);
                        return;
                    }
                }
                totalSent += iResult;
                cout << "\rSent: " << setprecision(2)  << (totalSent * 100.0) / headerSent.dataLength << "%";
                length -= iResult;
            }
            cout << "\rSent " << setprecision(2) << 100.0 << "%)" << " bytes of data completed\n";

            return;
        }
    }
    else
    {
        cout << "Error open file, plese retry.\n";
        return;
    }

    return;
}