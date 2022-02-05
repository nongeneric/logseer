#pragma once

#ifdef __MINGW32__
#include <winsock2.h>
#include <mutex>

#define LOGSEER_SOCKET_TYPE SOCKET
#define LOGSEER_CLOSE_SOCKET closesocket
#define LOGSEER_SHUTDOWN_RDWR SD_BOTH
#define LOGSEER_UNLINK DeleteFileA
#define LOGSEER_ERRNO WSAGetLastError()
constexpr SOCKET LOGSEER_INVALID_SOCKET = INVALID_SOCKET;
struct sockaddr_un
{
    unsigned short sun_family;
    char sun_path[108];
};

inline void LOGSEER_SOCKET_INIT() {
    static std::once_flag flag;
    std::call_once(flag, [] {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2,2), &wsaData);
    });
}

inline int LOGSEER_SOCKET_READ(SOCKET s, void* buff, int count) {
    DWORD read = 0;
    ReadFile((HANDLE)s, buff, count, &read, NULL);
    return read;
}

inline int LOGSEER_SOCKET_WRITE(SOCKET s, const void* buff, int count) {
    DWORD written = 0;
    WriteFile((HANDLE)s, buff, count, &written, NULL);
    return written;
}
#else
#include <sys/socket.h>
#include <sys/un.h>

#define LOGSEER_SOCKET_TYPE int
#define LOGSEER_CLOSE_SOCKET close
constexpr int LOGSEER_INVALID_SOCKET = -1;
#define LOGSEER_SHUTDOWN_RDWR SHUT_RDWR
#define LOGSEER_UNLINK unlink
#define LOGSEER_SOCKET_INIT()
#define LOGSEER_ERRNO errno
#define LOGSEER_SOCKET_WRITE write
#define LOGSEER_SOCKET_READ read
#endif
