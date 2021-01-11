#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#define max_connection 5
#define PORT_NO 10570
#define BUF_SIZE 4096
#define MAX_LINE_LEN 1024

int parse_input(FILE *fp);

/* 個人データ用構造体 ****************************************************** */
struct date
{
    int y;
    int m;
    int d;
};

struct profile
{
    int id;
    char name[70];
    struct date birthday;
    char home[70];
    char *comment;
};

/* グローバル変数 ********************************************************** */
struct profile profile_data[1000000];
int nprofiles = 0;

/* ************************************************************************* *
 * 文字列s中の文字fromを文字toで置き換える関数
 * ************************************************************************* */
void subst(char *s, char from, char to)
{
    int n;

    for (n = 0; s[n] != '\0'; n++)
    {
        if (s[n] == from)
        {
            s[n] = to;
        }
    }
}

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

/* ************************************************************************* *
 * 入力文字列lineを解析してデータを登録する関数
 * ************************************************************************* */
struct profile *
add_profile(struct profile *p,
            char *line)
{
    char *data[5], *birth[3];

    split(line, data, ',', 5);

    p->id = atoi(data[0]);
    strcpy(p->name, data[1]);
    strcpy(p->home, data[3]);
    p->comment = (char *)malloc(sizeof(char) * (strlen(data[4]) + 1));
    strcpy(p->comment, data[4]);

    split(data[2], birth, '-', 3);
    p->birthday.y = atoi(birth[0]);
    p->birthday.m = atoi(birth[1]);
    p->birthday.d = atoi(birth[2]);
    // sleep(0.5);
    return p;
}
/*クライアント側にメッセージを送信する*/
void send_message(int new_s, char message[BUF_SIZE])
{
    int send_byte = send(new_s, message, strlen(message), 0);
    if (send_byte < 0)
    {
        printf("応答メッセージの送信に失敗\n");
    }
    else
    {
        printf("応答メッセージの送信に成功\n");
    }
}

int get_line(FILE *fp, char *line)
{
    if (fgets(line, MAX_LINE_LEN + 1, fp) == NULL)
    {
        return 0;
    }
    subst(line, '\n', '\0');

    return 1;
}

/* 終了コマンド(%Q) ******************************************************** */
void command_quit(int new_s)
{
    char message[BUF_SIZE];
    memset(message, 0, BUF_SIZE);
    sprintf(message, "%s", "Q");
    printf("クライアントとの接続を切断します\n");
    send_message(new_s, message);
    // exit(0);
}

/* チェックコマンド(%C) **************************************************** */
void command_check(int new_s)
{
    char message[BUF_SIZE];
    memset(message, 0, BUF_SIZE);
    sprintf(message, "%s,%d", "C", nprofiles);
    printf("登録されているデータは%s件\n", message);
    send_message(new_s, message);
}

/* ************************************************************************* *
 * 登録データを１つ表示する関数
 * ************************************************************************* */
void print_profile(struct profile *p)
{
    printf("Id    : %d\n", p->id);
    printf("Name  : %s\n", p->name);
    printf("Birth : %04d-%02d-%02d\n", p->birthday.y, p->birthday.m, p->birthday.d);
    printf("Addr  : %s\n", p->home);
    printf("Com.  : %s\n", p->comment);
}
/* ************************************************************************* *
 * データをCSV形式で出力する関数
 * ************************************************************************* */
void print_profile_csv(FILE *fp, struct profile *p)
{
    fprintf(fp, "%d,", p->id);
    fprintf(fp, "%s,", p->name);
    fprintf(fp, "%04d-%02d-%02d,", p->birthday.y, p->birthday.m, p->birthday.d);
    fprintf(fp, "%s,", p->home);
    fprintf(fp, "%s\n", p->comment);
}

/* プリントコマンド(%P) **************************************************** */
void command_print(int new_s, struct profile *p,
                   int num)
{
    int start = 0, end = nprofiles;
    int n;

    if (num > 0 && num < nprofiles)
    { //登録している情報より少ないとき
        end = num;
    }
    else if (num < 0 && num + end > 0)
    {
        start = num + end;
    }

    int flag = 0;
    for (n = start; n < end; n++)
    {
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        int second = 0.5;
        if (flag == 0)
        {

            sprintf(message, "%s,%d", "P", end);
            send_message(new_s, message);
            flag = 1;
            n--;
            sleep(second);
        }
        else
        {
            sprintf(message, "%d,%s,%04d-%02d-%02d,%s,%s",
                    p[n].id,
                    p[n].name,
                    p[n].birthday.y,
                    p[n].birthday.m,
                    p[n].birthday.d,
                    p[n].home,
                    p[n].comment);
            send_message(new_s, message);
            sleep(second);
        }
    }
    printf("Print終了\n");
}

/* 読み込みコマンド(%R) **************************************************** */
void command_read(int new_s, struct profile *p,
                  char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "%%R: file open error %s.\n", filename);
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s,%s", "R", "E");
        send_message(new_s, message);
    }
    else
    {
        printf("Read開始\n");
        sleep(3);
        while (parse_input(fp))
            ;
        fclose(fp);
        printf("Read終了\n");
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s", "R");
        send_message(new_s, message);
    }
}

/* 書き出しコマンド(%W) **************************************************** */
void command_write(int new_s, struct profile *p,
                   char *filename)
{
    int n;
    char path[] = "../client/";
    strcat(path, filename);
    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "%%W: file write error %s.\n", filename);
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s,%s", "W", "E");
        send_message(new_s, message);
    }
    else
    {
        printf("Write開始\n");
        for (n = 0; n < nprofiles; n++)
        {
            print_profile_csv(fp, &p[n]);
        }
        fclose(fp);
        printf("Write終了\n");
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s", "W");
        send_message(new_s, message);
    }
}

/* ************************************************************************* *
 * IDの値を文字列に変換する関数
 * ************************************************************************* */
void make_id_string(int id,
                    char *str)
{
    sprintf(str, "%d", id);
}

/* ************************************************************************* *
 * Date構造体の値を-区切りの文字列に変換する関数
 * ************************************************************************* */
void make_birth_string(struct date *birth,
                       char *str)
{
    sprintf(str, "%04d-%02d-%02d", birth->y, birth->m, birth->d);
}

/* 検索コマンド(%F) ******************************************************** */
void command_find(int new_s, struct profile *p,
                  char *keyword)
{
    char id[8], birth[11];
    int n;
    char message[BUF_SIZE];
    int hit_num = 0;

    memset(message, 0, BUF_SIZE);
    sprintf(message, "%s", "F");
    send_message(new_s, message);
    sleep(1);

    for (n = 0; n < nprofiles; n++)
    {
        make_id_string(p[n].id, id);
        make_birth_string(&p[n].birthday, birth);
        if (strcmp(id, keyword) == 0 ||
            strcmp(birth, keyword) == 0 ||
            strcmp(p[n].name, keyword) == 0 ||
            strcmp(p[n].home, keyword) == 0)
        {
            printf("検索該当あり\n");
            hit_num++;
            sleep(1);
            char message[BUF_SIZE];
            memset(message, 0, BUF_SIZE);
            sprintf(message, "%d,%s,%04d-%02d-%02d,%s,%s",
                    p[n].id,
                    p[n].name,
                    p[n].birthday.y,
                    p[n].birthday.m,
                    p[n].birthday.d,
                    p[n].home,
                    p[n].comment);
            send_message(new_s, message);
        }
    }
    sleep(1);
    char final_message[BUF_SIZE];
    memset(final_message, 0, BUF_SIZE);
    if (hit_num)
    {
        sprintf(final_message, "%s", "end");
    }
    else
    {
        sprintf(final_message, "%s", "no_hit");
    }
    printf("検索終了\n");
    send_message(new_s, final_message);
}

/* ************************************************************************* *
 * 各項目でのソート関数
 * ************************************************************************* */
int sort_by_id(struct profile *p1,
               struct profile *p2)
{
    return p1->id - p2->id;
}

int sort_by_name(struct profile *p1,
                 struct profile *p2)
{
    return strcmp(p1->name, p2->name);
}

int sort_by_birthday(struct profile *p1,
                     struct profile *p2)
{
    if (p1->birthday.y != p2->birthday.y)
        return p1->birthday.y - p2->birthday.y;
    if (p1->birthday.m != p2->birthday.m)
        return p1->birthday.m - p2->birthday.m;
    if (p1->birthday.d != p2->birthday.d)
        return p1->birthday.d - p2->birthday.d;

    return 0;
}

int sort_by_home(struct profile *p1,
                 struct profile *p2)
{
    return strcmp(p1->home, p2->home);
}

int sort_by_comment(struct profile *p1,
                    struct profile *p2)
{
    return strcmp(p1->comment, p2->comment);
}

/* ************************************************************************* *
 * クイックソート
 * ************************************************************************* */
void quick_sort(struct profile *p,
                int start,
                int end,
                int (*compare_func)(struct profile *p1,
                                    struct profile *p2))
{
    struct profile tmp, key;
    int left, right;

    /* キーを決める */
    key = p[(start + end) / 2];

    /* 交換するデータを交換する */
    left = start;
    right = end;

    while (1)
    {
        while (compare_func(&key, &p[left]) > 0)
            left++;
        while (compare_func(&key, &p[right]) < 0)
            right--;
        if (left >= right)
            break;
        tmp = p[left];
        p[left] = p[right];
        p[right] = tmp;

        fprintf(stderr, "debug-begin: left-right\n");
        print_profile(&p[left]);
        print_profile(&p[right]);
        fprintf(stderr, "debug-end:\n");
    }
    /* 左部分をさらにソート */
    if (start < left - 1)
        quick_sort(p, start, left - 1, compare_func);

    /* 右部分をさらにソート */
    if (end > right + 1)
        quick_sort(p, right + 1, end, compare_func);
}

/* ************************************************************************* *
 * ソート関数の配列
 * ************************************************************************* */
int (*compare_function[])(struct profile *p1,
                          struct profile *p2) =
    {
        sort_by_id,
        sort_by_name,
        sort_by_birthday,
        sort_by_home,
        sort_by_comment};

/* 整列コマンド(%S) ******************************************************** */
void command_sort(int new_s, struct profile *p,
                  int column)
{
    if (column > 0 && column < 6)
    {
        quick_sort(profile_data, 0, nprofiles - 1, compare_function[column - 1]);
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s", "S");
        send_message(new_s, message);
    }
    else
    {
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s,%s", "S", "E");
        send_message(new_s, message);
    }
}

/* ************************************************************************* *
 * コマンド分岐関数
 * ************************************************************************* */
void exec_command(int new_s, char command,
                  char *parameter)
{
    switch (command)
    {
    case 'Q':
        command_quit(new_s);
        break;
    case 'C':
        command_check(new_s);
        break;
    case 'P':
        command_print(new_s, profile_data, atoi(parameter));
        break;
    case 'R':
        command_read(new_s, profile_data, parameter);
        break;
    case 'W':
        command_write(new_s, profile_data, parameter);
        break;
    case 'F':
        command_find(new_s, profile_data, parameter);
        break;
    case 'S':
        command_sort(new_s, profile_data, atoi(parameter));
        break;
    default:
        printf("Invalid command '%c' was found.\n", command);
        char message[BUF_SIZE];
        memset(message, 0, BUF_SIZE);
        sprintf(message, "%s", "Z");
        send_message(new_s, message);
        break;
    }
}

/* ************************************************************************* *
 * 入力文字列の解析(ファイルから読み込む用)
 * ************************************************************************* */
int parse_input(FILE *fp)
{ //FILE *fp
    char line[1024];
    int new_s;
    if (fgets(line, 1024, fp) == NULL)
        return 0;

    subst(line, '\n', '\0');
    if (line[0] == '%')
    {
        exec_command(new_s, line[1], &line[3]);
    }
    else
    {
        // printf("hellow world\n");
        add_profile(&profile_data[nprofiles], line);
        nprofiles++;
    }
    return 1;
}

/* ************************************************************************* *
 * 入力文字列の解析(ファイルから読み込む用)
 * ************************************************************************* */
int parse_input_from_stdin(int new_s, char command[BUF_SIZE])
{ //FILE *fp
    // char line[BUF_SIZE];
    // line = command;
    subst(command, '\n', '\0');
    if (command[0] == '%')
    {
        exec_command(new_s, command[1], &command[3]);
    }
    else
    {
        add_profile(&profile_data[nprofiles], &command[0]);
        nprofiles++;
        send_message(new_s, "register");
    }
    return 1;
}
/* ************************************************************************* *
 * メイン関数
 * ************************************************************************* */
int main(int argc, char *argv[])
{
    int s, new_s, check_bind, check_listen;
    struct sockaddr_in sa;
    char buf[BUF_SIZE];
    char message[BUF_SIZE];
    char len[BUF_SIZE];

    //ソケットを作成する
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        printf("ソケットの作成に失敗しました\n");
    }
    else
    {
        printf("ソケットの作成に成功しました\n");
    }

    //ソケットに名前を付ける
    memset((char *)&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;                /* インターネットドメイン */
    sa.sin_addr.s_addr = htonl(INADDR_ANY); /* どのIPアドレスでも接続OK */
    sa.sin_port = htons(PORT_NO);           /* 接続待ちのポート番号を設定 */
    check_bind = bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr));
    if (check_bind < 0)
    {
        printf("ソケットに名前を付けることに失敗しました\n");
        return -1;
    }
    else
    {
        printf("ソケットに名前を付けることに成功しました\n");
    }

    while (1)
    {
        //接続要求を待つ
        check_listen = listen(s, max_connection);
        if (check_listen < 0)
        {
            printf("接続要求を待っていません\n");
            return -1;
        }
        else
        {
            printf("接続要求を待っています\n");
        }

        //接続要求を受け付ける
        socklen_t size = sizeof(struct sockaddr);
        new_s = accept(s, (struct sockaddr *)&sa, &size);
        if (new_s < 0)
        {
            printf("接続要求を受け付けませんでした\n");
            return -1;
        }
        else
        {
            printf("接続要求を受けました\n");
        }

        while (1)
        {
            //メッセージを受信する
            int recv_byte = recv(new_s, buf, BUF_SIZE, 0);
            if (recv_byte < 0)
            {
                // printf("receive失敗\n");
            }
            else if (recv_byte == 0)
            {
                printf("接続先がシャットダウン\n");
                break;
            }
            else
            {
                printf("要求メッセージの受信に成功\n");
                memset(message, 0, BUF_SIZE);
                sprintf(message, "%s", buf);

                //コマンド処理
                parse_input_from_stdin(new_s, message);
                memset(message, 0, BUF_SIZE);
            }
            sleep(1);
            printf("処理終了\n");
            printf("-------------------------------------------\n");
        }
        close(new_s);
    }
}