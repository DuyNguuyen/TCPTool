# TCPTool
## Build chương trình:
- Khuyến khích build chương trình với Visual Studio 2019.
## Chạy chương trình:
- Nên khởi chạy **ReceiveData.exe** trước **SendData.exe**.
- Khởi chạy **ReceiveData.exe** với các lựa chọn:
```
Allowed options:
  -h [ --help ]                      produce help message.
  -p [ --port ] arg (=27600)         Set port to listen for connection.
  -o [ --out ] arg (=D:\socket out\) Set directory to store received file.
```
- Khởi chạy **SendData.exe** với các lựa chọn:
```
Allowed options:
  -h [ --help ]                 produce help message
  -d [ --des ] arg (=127.0.0.1) Set ip address to send data to
  -p [ --port ] arg (=27600)    Set port to sent data to
```
- Sau khi khởi chạy **SendData.exe**, chương trình sẽ hiện:
```
1 - Send text to receiver.
2 - Send file to receiver.
0 - Exit.
Please select option:
```
- Nhập **1** để gửi văn bản đến bên nhận, nhập **2** để gửi file đến bên nhận hoặc nhập **0** để kết thúc chương trình sau đó nhấn **Enter**.
- Nếu nhập **1**, tiếp tục nhập đoạn văn bản muôn gửi đến bên nhận sau đó nhấn **Enter**
- Nếu nhập **2**, tiếp tục nhập đường dẫn đến tập tin cần gửi (ví dụ: C:\test:\test.txt) sau đó nhấn **Enter**. 
  cuối cùng nhập kích thước của buffer mỗi lần gửi rồi nhấn **Enter**.
## Đường dẫn tài liệu tham khảo
- [Windows Sockets 2](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2)
- [GetFileAttributesA function](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributes)
- [XOR Cipher](https://www.geeksforgeeks.org/xor-cipher/)
