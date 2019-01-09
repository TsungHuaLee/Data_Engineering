#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
#include <unordered_map>
using namespace std;

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


// A utility function to get maximum of two integers
int max (int a, int b) { return (a > b)? a: b; }

// The preprocessing function for Boyer Moore's
// bad character heuristic
void badCharHeuristic( wchar_t *str, int size, unordered_map<wchar_t, int>& badchar)
{
    int i;
    /* Fill the actual value of last occurrence
     of a character */
    for (i = 0; i < size; i++)
    {
        badchar[str[i]] = i;
    }
}

/* A pattern searching function that uses Bad
   Character Heuristic of Boyer Moore Algorithm */
int Boyer_Moore( wchar_t *txt,  wchar_t *pat)
{
    int re = 0;
    int m = wcslen(pat);
    int n = wcslen(txt);

    unordered_map<wchar_t, int> badchar;

    /* Fill the bad character array by calling
       the preprocessing function badCharHeuristic()
       for given pattern */
    badCharHeuristic(pat, m, badchar);

    int s = 0;  /* s is shift of the pattern with
                 respect to text */
    while(s <= (n - m))
    {
        int j = m-1;

        /* Keep reducing index j of pattern while
           characters of pattern and text are
           matching at this shift s */
        while(j >= 0 && pat[j] == txt[s+j])
            j--;

        /* If the pattern is present at current
           shift, then index j will become -1 after
           the above loop */
        if (j < 0)
        {
            //printf("\n pattern occurs at shift = %d\n", s);
            re++;
            /* Shift the pattern so that the next
               character in text aligns with the last
               occurrence of it in pattern.
               The condition s+m < n is necessary for
               the case when pattern occurs at the end
               of text */
            s += (s+m < n)? m-badchar[txt[s+m]] : 1;
        }
        else
            /* Shift the pattern so that the bad character
               in text aligns with the last occurrence of
               it in pattern. The max function is used to
               make sure that we get a positive shift.
               We may get a negative shift if the last
               occurrence  of bad character in pattern
               is on the right side of the current
               character. */
            s += max(1, j - badchar[txt[s+j]]);
    }
    return re;
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
        return 10;
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
            return 1;
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
    wchar_t *buffer = NULL, *ret = NULL;
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

            /* searching exclude!!! */
            match = 0; score = 0;
            for(int idx = 0; idx < search->ex_num && score == 0; idx++)
            {
                score += Boyer_Moore(oneNews.title, search->exclude[idx]);
                score += Boyer_Moore(oneNews.context, search->exclude[idx]);
            }
            if(score != 0)
                continue;

            /* searching exclude!!! */
            for(int idx = 0; idx < search->in_num; idx++)
            {
                int temp = Boyer_Moore(oneNews.title, search->include[idx]) * 1000;
                temp += Boyer_Moore(oneNews.context, search->include[idx]) * 100;
                if(temp == 0)
                    break;
                score += temp;
                match++;
            }
            if(match != search->in_num)
                continue;

            /* searching favor!!! */
            for(int idx = 0; idx < search->fav_num; idx++)
            {
                score += matching(oneNews.title, search->favor[idx]) * 300;
                score += matching(oneNews.context, search->favor[idx]) * 30;
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
    //printf("cnt = %lu\n", cnt);

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
        printf("%ls\t%ls\t%d\t%ls\n", totalRec[i].title, totalRec[i].url, totalRec[i].score, totalRec[i].context);
    }
    free(temp);
    fclose(fout);
    return 0;
}
