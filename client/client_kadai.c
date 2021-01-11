#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUF_SIZE 4096

/* ************************************************************************* *
 * 文字列strを区切り文字separatorで最大nitems個に分割して、分割した文字列の
 * それぞれの先頭アドレスを配列retに代入する関数
 * ************************************************************************* */
int split(char *str, char *ret[], char separator, int nitems)
{
    int count = 0, n;

    ret[count++] = str;

    for (n = 0; str[n] != '\0' && count < nitems; n++)
    {
        if (str[n] == separator)
        {
            str[n] = '\0';
            ret[count++] = str + n + 1;
        }
    }
    return count;
}

int main(int argc, char *argv[])
{
    struct hostent *hostent;
    struct sockaddr_in sa;
    int s;
    int PORT_NO = 10570;
    char buf[BUF_SIZE];
    char message[BUF_SIZE];
    char msg1[BUF_SIZE / 2];
    char msg2[BUF_SIZE / 2];
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
    //AF_INET:IPv4 インターネットプロトコル
    //SOCK_STREAM:TCP用のソケットを指定
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
    if (connect(s, (struct sockaddr *)&sa, sizeof(struct sockaddr)) != 0)
    {
        printf("接続失敗\n");
    }
    else
    {
        printf("接続成功\n");
    }
    while (1)
    {
        printf("\n");
        printf("コマンドを入力してください\n");
        while (1)
        {
            // //要求メッセージを送信
            memset(message, 0, BUF_SIZE);
            memset(buf, 0, BUF_SIZE);
            memset(msg1, 0, BUF_SIZE);
            memset(msg2, 0, BUF_SIZE);
            read(0, msg1, BUF_SIZE - 2); //0が入力
            sprintf(message, "%s\r\n", msg1);
            if (message != NULL)
            {
                break;
            }
        }
        if (message != NULL)
        {
            int send_byte = send(s, message, strlen(message), 0);
            if (send_byte < 0)
            {
                printf("要求メッセージの送信に失敗\n");
            }

            memset(message, 0, BUF_SIZE);

            // //応答メッセージを受信
            int recv_byte = recv(s, buf, BUF_SIZE, 0);
            if (recv_byte < 0)
            {
                printf("応答メッセージの受信に失敗\n");
            }
            else if (recv_byte == 0)
            {
                printf("接続先がシャットダウン\n");
            }
            else
            {
                // printf("応答メッセージの受信に成功\n");
                char *command[2];
                split(buf, command, ',', 2);

                if (strcmp(command[0], "Q") == 0)
                {
                    exit(0);
                }
                else if (strcmp(command[0], "C") == 0)
                {
                    printf("%s profile(s)\n", command[1]);
                }
                else if (strcmp(command[0], "P") == 0)
                {
                    int n = atoi(command[1]);
                    for (int i = 0; i < n; i++)
                    {
                        int recv_byte = recv(s, buf, BUF_SIZE, 0);
                        char *data[5];
                        split(buf, data, ',', 5);
                        printf("-----------------------------------------------------\n");
                        printf("Id    : %s\n", data[0]);
                        printf("Name  : %s\n", data[1]);
                        printf("Birth : %s\n", data[2]);
                        printf("Addr  : %s\n", data[3]);
                        printf("Com.  : %s\n", data[4]);
                        printf("------------------------------------------------------\n");
                    }
                }
                else if (strcmp(command[0], "R") == 0)
                {
                    if (strcmp(command[1], "E") == 0)
                    {
                        printf("DBにデータを準備することができませんでした。\nファイル名を確認してください\n");
                    }
                    else
                    {
                        printf("DBにデータを準備できました\n");
                    }
                }
                else if (strcmp(command[0], "W") == 0)
                {
                    if (strcmp(command[1], "E") == 0)
                    {
                        printf("指定されたファイルが存在しないため書き込めません\nデータが登録されているか，またファイルの名前が合っているかを確認してください\n");
                    }
                    else
                    {
                        printf("DBからデータを取得，ファイルへの書き込みを完了しました\n");
                    }
                }
                else if (strcmp(command[0], "F") == 0)
                {
                    printf("検索結果を表示します\n");
                    while (1)
                    {
                        memset(buf, 0, BUF_SIZE);
                        int recv_byte = recv(s, buf, BUF_SIZE, 0);
                        if (strcmp(buf, "end") == 0)
                        {
                            break;
                        }
                        else if (strcmp(buf, "no_hit") == 0)
                        {
                            printf("検索条件に当てはまるデータは存在しませんでした\n");
                            break;
                        }
                        char *data[5];
                        split(buf, data, ',', 5);
                        printf("------------------------------------------------------\n");
                        printf("Id    : %s\n", data[0]);
                        printf("Name  : %s\n", data[1]);
                        printf("Birth : %s\n", data[2]);
                        printf("Addr  : %s\n", data[3]);
                        printf("Com.  : %s\n", data[4]);
                        printf("-------------------------------------------------------\n");
                    }
                }
                else if (strcmp(command[0], "S") == 0)
                {
                    if (strcmp(command[1], "E") == 0)
                    {
                        printf("カラム番号が存在しません\n1:ID\n2:氏名\n3:年月日\n4:住所\n5:備考\nで指定してくだだい\n");
                    }
                    else
                    {
                        printf("ソートが完了しました。\n");
                    }
                }
                else if (strcmp(command[0], "register") == 0)
                {
                    printf("データを登録できました\n");
                }
                else if (strcmp(command[0], "Z") == 0)
                {
                    printf("コマンドが存在しません\nコマンドを確認してください\n");
                }
            }
        }
    }
    // //応答メッセージを処理
    // //ソケットの削除
    close(s);
}
