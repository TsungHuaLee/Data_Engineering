#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>

const size_t bufferSize = 10000;
const size_t chMinLen = 6;
const char *fileNamePre = "ettoday";
// 中文 Unicode 範圍
const unsigned long minChinese = 0x4E00;
const unsigned long maxChinese = 0x9FFF;
//判斷斷句符號
const wchar_t *delim = L"。！？!?\r\b\t\n";

typedef struct news
{
    wchar_t *url;
    wchar_t *title;
    wchar_t *context;
}newsRecord;
//判斷是否中文
int isChinese(wchar_t c)
{
    unsigned long tmp = (unsigned long)c;
    return ((tmp >= minChinese) && (tmp <= maxChinese)) ? 1 : 0;
}
//斷句
int breakSentence(wchar_t *mainContext, wchar_t ***sentences)
{
    wchar_t **buffer = NULL;
    wchar_t *token = NULL, *state = NULL;           //wcstok
    int cnt = 0, size = 100;
    buffer = (wchar_t**)malloc(sizeof(wchar_t*)*size);
    for(token = wcstok(mainContext, delim, &state); token!= NULL; token = wcstok(NULL, delim, &state))    //token 存取該次切的,state 存取剩下的
    {
        int chCnt = 0;

        for(int i = 0; i < wcslen(token); i++)  //determine chinese length > 5
        {
            chCnt += isChinese(token[i]);
            if(chCnt > chMinLen)
                break;
        }

        if(cnt >= size)                         //確認大小
        {
            size *= 2;
            wchar_t **tempPtr = (wchar_t**)malloc(sizeof(wchar_t*)*size);
            memcpy(tempPtr, buffer, sizeof(wchar_t*)*cnt);
            free(buffer);
            buffer = tempPtr;
            tempPtr = NULL;
        }
        buffer[cnt] = NULL;
        buffer[cnt] = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(token)+1));
        if (buffer[cnt]==NULL) exit(1);
        wcscpy(buffer[cnt], token);
        cnt++;
    }
    *sentences = buffer;
    return cnt;
}

void read()
{
    FILE *fin = fopen("ettoday0.rec", "rb");
    if(fin == NULL) exit(1);
    FILE *fout = fopen("sentences.txt", "wb");
    if(fout == NULL) exit(1);
    wchar_t *buffer = NULL;
    wchar_t **sentences = NULL;
    newsRecord oneNews;
    buffer = (wchar_t*)malloc(sizeof(wchar_t)*(bufferSize + 10));
    while(fgetws(buffer, bufferSize, fin) != NULL)  //go through one rec file
    {
        if(wcsncmp(buffer, L"@GAISRec:", 9) != 0) continue;    //單筆資料開頭
        fgetws(buffer,bufferSize, fin);     //url
        buffer[wcslen(buffer)-1] = '\0';    //omit "\n"
        oneNews.url = (wchar_t*)malloc(sizeof(wchar_t)*wcslen(buffer));
        wcscpy(oneNews.url, buffer + 3);    //omit "@U:"
        fgetws(buffer, bufferSize, fin);    //title
        buffer[wcslen(buffer)-1] = '\0';    //omit "\n"
        oneNews.title = (wchar_t*)malloc(sizeof(wchar_t)*wcslen(buffer));
        wcscpy(oneNews.title, buffer + 3);  //omit "@T:"
        fgetws(buffer, bufferSize, fin);    //omit "@B:"
        fgetws(buffer, bufferSize, fin);    //main context
        buffer[wcslen(buffer)-1] = '\0';    //omit "\n"
        oneNews.context = (wchar_t*)malloc(sizeof(wchar_t)*wcslen(buffer));
        wcscpy(oneNews.context, buffer + 33); //omit begining space
        int strCnt = breakSentence(oneNews.context, &sentences);    //!!
        printf("%d\n", strCnt);

        for (int i=0; i<strCnt; ++i)
        {
            fwprintf(fout, L"%ls\t%ls\t%ls\n", sentences[i], oneNews.url, oneNews.title); // 寫入句子到檔案
            free(sentences[i]);
            sentences[i]=NULL;
        }

        free(oneNews.url); oneNews.url = NULL;
        free(oneNews.title); oneNews.title = NULL;
        free(oneNews.context); oneNews.context = NULL;
    }
    free(buffer);
    fclose(fin);
    fclose(fout);
}

int main()
{
    setlocale(LC_CTYPE, "");
    read();
    return 0;
}
