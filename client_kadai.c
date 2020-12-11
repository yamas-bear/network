#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUF_SIZE 4096
int main(int argc, char *argv[])
{
    struct hostent *hostent;
    struct sockaddr_in sa;
    int s;
    int PORT_NO = 10570;
    char buf[BUF_SIZE];
    char message[BUF_SIZE];
    //通信相手のIPアドレスを取得
    hostent = gethostbyname(argv[1]);
    if (hostent == NULL)
    {
        printf("IPアドレスの取得失敗\n");
    }
    else
    {
        printf("IPアドレスの取得成功\n");
    }

    //ソケットの作成
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        printf("ソケットの作成に失敗しました\n");
    }
    else
    {
        printf("ソケットの作成に成功しました\n");
    }
    sa.sin_family = hostent->h_addrtype; // host address type
    sa.sin_port = htons(PORT_NO);        // port number
    bzero((char *)&sa.sin_addr, sizeof(sa.sin_addr));
    memcpy((char *)&sa.sin_addr, (char *)hostent->h_addr, hostent->h_length);

    // //接続の設立
    // struct sockaddr_in sa;
    if (connect(s, (struct sockaddr *)&sa, sizeof(struct sockaddr)) == 0)
    {
        printf("接続成功\n");
    }
    else
    {
        printf("接続失敗\n");
    }

    // message = "GET /index.html";

    // while (1)
    // {
    // //要求メッセージを送信
    memset(message, 0, BUF_SIZE);
    memset(buf, 0, BUF_SIZE);
    sprintf(message, "%s", argv[2]);
    // sprintf(message, "%s", std_in);
    int send_byte = send(s, message, strlen(message), 0);
    if (send_byte < 0)
    {
        printf("sendに失敗\n");
    }
    memset(message, 0, BUF_SIZE);

    // //応答メッセージを受信
    int recv_byte = recv(s, buf, BUF_SIZE, 0);
    if (recv_byte < 0)
    {
        printf("receive失敗\n");
    }
    else if (recv_byte == 0)
    {
        printf("接続先がシャットダウン\n");
    }
    else
    {
        printf("-------------------------------------\n");
        printf("%s\n", buf);
        printf("-------------------------------------\n");

        printf("receive成功\n");
    }
    // }
    // //応答メッセージを処理
    // //ソケットの削除
    close(s);
}
