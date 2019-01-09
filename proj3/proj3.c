#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
const size_t bufferSize = 10000;
const char* files[] = {"./data/ettoday0.rec", "./data/ettoday1.rec", "./data/ettoday2.rec", "./data/ettoday3.rec", "./data/ettoday4.rec", "./data/ettoday5.rec"};
const size_t fileNum = 2;
char outFile[15] = {"final.rec"};
int k_error = 1;
size_t recNum = 100;
typedef struct news
{
    int score;
    wchar_t *url;
    wchar_t *title;
    wchar_t *context;
}newsRecord;
typedef struct Srch
{
    wchar_t** exclude;
    wchar_t** include;
    wchar_t** favor;
    int ex_num;
    int in_num;
    int fav_num;
}srchStruct;

void setParameter(const int argc, const char **argv, srchStruct* search)
{
    search->exclude = (wchar_t**)malloc(sizeof(wchar_t*) * (argc));
    search->include = (wchar_t**)malloc(sizeof(wchar_t*) * (argc));
    search->favor = (wchar_t**)malloc(sizeof(wchar_t*) * (argc));
    search->ex_num = 0; search->in_num = 0; search->fav_num = 0;
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            search->exclude[search->ex_num] = (wchar_t*)malloc(sizeof(wchar_t) * strlen(argv[i+1]));
            mbstowcs(search->exclude[search->ex_num], (*(argv + i + 1)), strlen(*(argv + i + 1)) );
            search->ex_num++;
            i++;
            continue;
        }
        else if(argv[i][0] == '+')
        {
            search->include[search->in_num] = (wchar_t*)malloc(sizeof(wchar_t) * strlen(argv[i+1]));
            mbstowcs(search->include[search->in_num], (*(argv + i + 1)), strlen(*(argv + i + 1)) );
            search->in_num++;
            i++;
            continue;
        }
        else
        {
            search->favor[search->fav_num] = (wchar_t*)malloc(sizeof(wchar_t) * strlen(argv[i]));
            mbstowcs(search->favor[search->fav_num], (*(argv + i)), strlen(*(argv + i)) );
            search->fav_num++;
        }
    }
    return ;
}

void formatLine(wchar_t *str)
{ // 假設輸入 str 只有一行內容
    wchar_t *ptr = str;
    while(ptr!=NULL && *ptr!=0)
    {
        if (*ptr=='\t' || *ptr=='\b' || *ptr=='\r') *ptr=' '; // 清除空格與其他東西
        if (*ptr=='\n') {  // 清除結尾換行符號
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

int matching(wchar_t *str, wchar_t * search)
{
    int col = wcslen(str) , row = wcslen(search);
    // totally matching
    wchar_t *test_match;
    test_match = wcsstr(str, search);
    if(test_match != NULL)
        return row;
    /* approximate match */
    int table[row + 1][col + 1];
    /* initialize table */
    memset(table, 0, sizeof(table));
    for(int i = 0; i <= col; i++)
        table[0][i] = 0;
    for(int i = 0; i <= row; i++)
        table[i][0] = i;
    /*calculate*/
    for(int j = 1; j <= col; j++)
        for(int i = 1;i <= row; i++)
        {
            table[i][j] = min(table[i-1][j-1] + (str[j-1] != search[i-1]), table[i-1][j] + 1, table[i][j-1] + 1);
        }

    for(int i = row,j = row; j <= col; j++)
    {
        if(table[i][j] <= k_error)
            return row - table[i][j];
    }
    return 0;
}

int cmp( const void *a ,const void *b)
{
    return (*(newsRecord *)a).score < (*(newsRecord *)b).score ? 1 : -1;
}
/*php 調用 c code wchar_t時有問題 , 解法https://mikecoder.cn/post/135/?fbclid=IwAR3h_N-i35t0_BKy51PANpeuhdJ_Tlv5ZB_IGaEn90yjsMWIAc41Z7QBJT4*/
size_t parse(newsRecord** results, srchStruct* search)
{
    wchar_t *buffer = NULL, *ret, *test_match;
    size_t cnt = 0;
    int match = 0, score = 0;
    newsRecord oneNews;
    newsRecord* totalRec = (newsRecord*)malloc(sizeof(newsRecord)*recNum);
    FILE *fin;

    for(int i = 0; i < fileNum; i++)
    {
        //printf("search file %d\n", i);
        fin = fopen(files[i], "rb");
        if(fin == NULL)
        {
            printf("file%d is not existed\n", i);
            return -1;
        }

        buffer = (wchar_t*)malloc(sizeof(wchar_t)*(bufferSize+10));
        while(fgetws(buffer, bufferSize, fin) != NULL)
        {
            if(wcsncmp(buffer, L"@GAISRec:", 9) != 0) continue;
            /*url*/
            ret = fgetws(buffer,bufferSize, fin);
            oneNews.url = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.url, buffer+3);
            /*title*/
            ret = fgetws(buffer, bufferSize, fin);
            oneNews.title = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.title, buffer+3);
            /*omit @B*/
            ret = fgetws(buffer, bufferSize, fin);
            /*main context*/
            ret = fgetws(buffer, bufferSize, fin);
            oneNews.context = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.context, buffer+3);

            /* searching!!! */
            match = 0; score = 0, test_match = NULL;
            for(int idx = 0; idx < search->ex_num; idx++)
            {
                test_match = wcsstr(oneNews.title, search->exclude[idx]);
                if(test_match != NULL)
                    break;
                test_match = wcsstr(oneNews.context, search->exclude[idx]);
                if(test_match != NULL)
                    break;
            }
            if(test_match!=NULL)
                continue;

            for(int idx = 0; idx < search->in_num; idx++)
            {
                int temp = matching(oneNews.title, search->include[idx]) * 100;
                temp += matching(oneNews.context, search->include[idx]) * 10;
                if(temp == 0)
                    break;
                score += temp;
                match++;
            }
            if(match != search->in_num)
                continue;

            for(int idx = 0; idx < search->fav_num; idx++)
            {
                score += matching(oneNews.title, search->favor[idx]) * 100;
                score += matching(oneNews.context, search->favor[idx]) * 10;
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

    /* duplicate robust space */
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
    setlocale(LC_CTYPE, "");

    newsRecord* totalRec;
    srchStruct search;
    setParameter(argc, argv, &search);
    if(!search.in_num && !search.fav_num)
    {
        printf("nothing to search!!!\n");
        return 2;
    }

    size_t cnt = parse(&totalRec, &search);
    printf("cnt = %lu\n", cnt);

    qsort(totalRec, cnt, sizeof(newsRecord), cmp);
    //printf("sorting!!!\n");

    FILE* fout = fopen(outFile, "wb");
    if(fout == NULL)
    {
        printf("output file not found!!!\n");
        return 1;
    }
    char delim = L'\t', newline = L'\n', *temp = (char*)malloc(sizeof(char)*1000000);
    int ret;
    for(int i = 0; i < cnt; i++)
    {
        /*
        ret = wcstombs ( temp, totalRec[i].title, sizeof(wchar_t)*wcslen(totalRec[i].title) ); //title
        fwrite(temp, sizeof(char), strlen(temp), fout);
        fwrite(&delim, sizeof(char), 1, fout);
        ret = wcstombs ( temp, totalRec[i].url, sizeof(wchar_t)*wcslen(totalRec[i].url) ); //url
        fwrite(temp, sizeof(char), strlen(temp), fout);
        fwrite(&delim, sizeof(char), 1, fout);
        fwrite(totalRec[i].score)
        ret = wcstombs ( temp, totalRec[i].context, sizeof(wchar_t)*wcslen(totalRec[i].context) ); // context
        fwrite(temp, sizeof(char), strlen(temp), fout);
        fwrite(&newline, sizeof(char), 1, fout);
        */
        printf("%ls\t%ls\t%d\t%ls\n", totalRec[i].title, totalRec[i].url, totalRec[i].score, totalRec[i].context);
    }
    free(temp);
    fclose(fout);
    return 0;
}
