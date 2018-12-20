#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
const size_t bufferSize = 10000;
const char* files[] = {"./data/test.rec", "./data/ettoday0.rec", "./data/ettoday1.rec", "./data/ettoday2.rec", "./data/ettoday3.rec", "./data/ettoday4.rec", "./data/ettoday5.rec"};
const size_t fileNum = 6;
wchar_t **search;
int search_num;
int *weight;
int k_error = 1;
size_t recNum = 100;
typedef struct news
{
    int score;
    wchar_t *url;
    wchar_t *title;
    wchar_t *context;
}newsRecord;

void setParameter(const int argc, const char **argv)
{
    search = (wchar_t**)malloc(sizeof(wchar_t*) * (argc + 1));
    weight = (int*)malloc(sizeof(int) * (argc + 1));
    search_num = 0;
    for(int i = 1; i < argc; i++)
    {
        search[i-1] = (wchar_t*)malloc(sizeof(wchar_t) * (strlen(argv[i]) + 1));
        char *temp = (char*)malloc(sizeof(char) * (strlen(argv[i]) + 1));
        if(argv[i][0] == '-')
        {
            strncpy(temp, (*(argv + i) + 1), strlen(argv[i]) - 1);
            mbstowcs(search[i-1], temp, strlen(temp));
            weight[i-1] = -1;
        }
        else if(argv[i][0] == '+')
        {
            strncpy(temp, (*(argv + i) + 1), strlen(argv[i]) - 1);
            mbstowcs(search[i-1], temp, strlen(temp));
            weight[i-1] = 1;
        }
        else
        {
            mbstowcs(search[i-1], argv[i], strlen(argv[i]));
            weight[i-1] = 1;
        }
        search_num++;
        free(temp);
    }
    return ;
}

void formatLine(wchar_t *str)
{ // 假設輸入 str 只有一行內容
    wchar_t *ptr = str;
    while(ptr!=NULL && *ptr!=0)
    {
        if (*ptr==L'\t' || *ptr==L'\b' || *ptr==L'\r') *ptr=L' '; // 清除空格與其他東西
        if (*ptr==L'\n') {  // 清除結尾換行符號
            *ptr=0;
            break;
        }
        ++ptr;
    }
}

int min(int a, int b, int c)
{
    int re = a;
    re = (re < b)?re:b;
    re = (re < c)?re:c;
    return re;
}
// return +N -> 要的, -N -> 不要的, 0 -> not found
int matching(wchar_t *str, int idx, int score){
    // totally matching
    wchar_t *test_match;
    test_match = wcsstr(str, search[idx]);
    if(test_match != NULL)
        return score * weight[idx];
    // approximate match
    int table[wcslen(search[idx]) + 1][wcslen(str) + 1];
    memset(table, 0, sizeof(table));
    //initialize table
    for(int i = 0; i <= wcslen(str); i++)
        table[0][i] = 0;
    for(int i = 0; i <= wcslen(search[idx]); i++)
        table[i][0] = i;
    //calculate
    for(int j = 1; j <= wcslen(str); j++)
        for(int i = 1;i <= wcslen(search[idx]); i++)
        {
            int neq = (str[j-1] != search[idx][i-1])?1:0;
            table[i][j] = min(table[i-1][j-1] + neq, table[i-1][j] + 1, table[i][j-1] + 1);
        }

    for(int i = wcslen(search[idx]),j = wcslen(search[idx]); j <= wcslen(str); j++)
    {
        //printf("\ni = %d j = %d table = %d, k_error = %d\n",i , j, table[i][j], k_error);
        if(table[i][j] <= k_error)
            return score * weight[idx] / 10;
    }
    return 0;
}
int cmp( const void *a ,const void *b)
{
    return (*(newsRecord *)a).score < (*(newsRecord *)b).score ? 1 : -1;
}
size_t parse(newsRecord** results)
{
    wchar_t *buffer = NULL;
    size_t cnt = 0;
    newsRecord oneNews;
    newsRecord* totalRec = (newsRecord*)malloc(sizeof(newsRecord)*recNum);
    FILE *fin;
    for(int i = 0; i < 2; i++)
    {
        printf("search file %d\n", i);
        fin = fopen(files[i], "rb");    //open file
        if(fin == NULL)
        {
            printf("file%d is not existed\n", i);
            return -1;
        }

        buffer = (wchar_t*)malloc(sizeof(wchar_t)*(bufferSize+10));
        while(fgetws(buffer, bufferSize, fin) != NULL)
        {
            if(cnt == 50)
                break;
            int score = 0;
            if(wcsncmp(buffer, L"@GAISRec:", 9) != 0) continue;
            fgetws(buffer,bufferSize, fin);     //url
            oneNews.url = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.url, buffer + 3);    //omit "@U:"
            fgetws(buffer, bufferSize, fin);    //title
            oneNews.title = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.title, buffer + 3);  //omit "@T:"
            // searching
            for(int idx = 0; idx < search_num; idx++)
            {
                score += matching(oneNews.title, idx, 50);
            }
            fgetws(buffer, bufferSize, fin);    //omit "@B:"
            fgetws(buffer, bufferSize, fin);    //main context
            oneNews.context = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.context, buffer);
            // searching
            for(int idx = 0; idx < search_num; idx++)
            {
                score += matching(oneNews.context, idx, 10);
            }
            if(score > 0)
            {
                if(cnt >= recNum)
                {
                    recNum *= 2;
                    totalRec = (newsRecord*)realloc(totalRec, recNum * sizeof(newsRecord));
                }
                oneNews.score = score;
                //printf("cnt = %lu, %d\n%ls\n%ls\n%ls\n", cnt,oneNews.score, oneNews.url, oneNews.title, oneNews.context);
                //fwprintf(fout, L"%d\n%ls\n%ls\n%ls\n", oneNews.score, oneNews.url, oneNews.title, oneNews.context);
                memcpy(&totalRec[cnt], &oneNews, sizeof(newsRecord));
                //printf("cnt = %lu, %d\n%ls\n%ls\n%ls\n", cnt,totalRec[cnt].score, totalRec[cnt].url, totalRec[cnt].title, totalRec[cnt].context);
                cnt++;
                oneNews.context=NULL;
                oneNews.url=NULL;
                oneNews.title=NULL;
                continue;
            }
            free(oneNews.context); oneNews.context=NULL;
            free(oneNews.url); oneNews.url=NULL;
            free(oneNews.title); oneNews.title=NULL;
        }
        fclose(fin);
        free(buffer);
    }

    // duplicate robust space
    newsRecord* tempRec = (newsRecord*)malloc(sizeof(newsRecord)*cnt);
    if (tempRec == NULL) exit(3);
    memcpy(tempRec, totalRec, sizeof(newsRecord)*cnt);
    free(totalRec);
    totalRec = tempRec;
    tempRec = NULL;

    *results = totalRec;
    //for(int i = 0; i < cnt; i++)
    //printf("cnt = %d, %d\n%ls\n%ls\n%ls\n", i,totalRec[i].score, totalRec[i].url, totalRec[i].title, totalRec[i].context);
    //printf("%d\n%ls\n%ls\n%ls\n",totalRec[0].score, totalRec[0].url, totalRec[0].title, totalRec[0].context);
    return cnt;
}

int main(const int argc, const char** argv)
{
    FILE* fout = fopen("final.rec", "wb");
    newsRecord* totalRec;
    setlocale(LC_CTYPE, "");
    setParameter(argc, argv);
    //printf("searching!!!\n");
    size_t cnt = parse(&totalRec);
    qsort(totalRec, cnt, sizeof(newsRecord), cmp);
    printf("sorting!!!\n");
    for(int i = 0; i < cnt; i++)
        fwprintf(fout, L"cnt = %d, %d\n%ls\n%ls\n%ls\n", i,totalRec[i].score, totalRec[i].url, totalRec[i].title, totalRec[i].context);
    return 0;
}
