#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
const size_t bufferSize = 10000;
const size_t chMinLen = 5;
const char* files[] = {"./data/ettoday0.rec", "./data/ettoday1.rec", "./data/ettoday2.rec", "./data/ettoday3.rec", "./data/ettoday4.rec", "./data/ettoday5.rec"};
const size_t fileNum = 6;
// 中文 Unicode 範圍
const unsigned long minChinese = 0x4E00;
const unsigned long maxChinese = 0x9FFF;
//判斷斷句符號
const wchar_t *delim = L"。！？!?\r\b\t\n ";

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

void formatLine(wchar_t *str) { // 假設輸入 str 只有一行內容
    wchar_t *ptr = str;
    while(ptr!=NULL && *ptr!=0) {
        if (*ptr==L'\t' || *ptr==L'\b' || *ptr==L'\r') *ptr=L' '; // 清除空格與其他東西
        if (*ptr==L'\n') {  // 清除結尾換行符號
            *ptr=0;
            break;
        }
        ++ptr;
    }
}

int tokenize(wchar_t ***results, wchar_t *str) {
    wchar_t **sentences=NULL, **s_ptr_t=NULL;
    wchar_t *buffer=NULL, *ptr=NULL;
    int cnt = 0;
    int cap = 1000; // 初始容量, 三十二個寬字元指標
    sentences = (wchar_t**) malloc(sizeof(wchar_t*)*cap);
    if (sentences==NULL) exit(1);
    buffer = wcstok(str, delim, &ptr);
    while(buffer!=NULL) {
        unsigned long chCnt = 0;
        wchar_t *ch_test_ptr = buffer;
        if (ch_test_ptr!=NULL && isChinese(ch_test_ptr[0])) { // 第一個字必須是中文字
            while(ch_test_ptr!=NULL && *ch_test_ptr!=0) {   // 計算中文數量
                chCnt += isChinese(*ch_test_ptr);
                ++ch_test_ptr;
            }
        }
        if (chCnt>=chMinLen) { // 一句中文必須達到6個字(含)以上
            if(cnt==cap) { // buffer 滿了
                cap *= 2; // 增加一倍容量
                sentences = realloc(sentences, sizeof(wchar_t*)*cap);
            }
            sentences[cnt] = NULL;
            sentences[cnt] = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer)+1));
            if (sentences[cnt]==NULL) exit(4);
            wcscpy(sentences[cnt], buffer);
            ++cnt;
        }
        buffer = wcstok(NULL, delim, &ptr);
    }
    // duplicate robust space
    s_ptr_t = NULL;
    s_ptr_t = (wchar_t**) malloc(sizeof(wchar_t*)*cnt);
    if (s_ptr_t==NULL) exit(3);
    memcpy(s_ptr_t, sentences, sizeof(wchar_t*)*cnt);
    free(sentences);
    sentences = s_ptr_t;
    s_ptr_t = NULL;

    *results = sentences;
    return cnt;
}

int parse()
{
    wchar_t *buffer = NULL;
    wchar_t **sentences = NULL;
    newsRecord oneNews;
    FILE *fin;
    FILE *fout = fopen("sentences.txt", "wb");
    if(fout == NULL)
    {
        printf("output file is not existed\n");
        exit(1);
    }
    for(int i = 0; i < 1; i++)
    {
        if(i > 0)
            break;

        fin = fopen(files[i], "rb");    //open file
        if(fin == NULL)
        {
            printf("file%d is not existed\n", i);
            return -1;
        }
        buffer = (wchar_t*)malloc(sizeof(wchar_t)*(bufferSize+10));

        while(fgetws(buffer, bufferSize, fin) != NULL)
        {
            if(wcsncmp(buffer, L"@GAISRec:", 9) != 0) continue;
            fgetws(buffer,bufferSize, fin);     //url
            oneNews.url = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.url, buffer + 3);    //omit "@U:"
            fgetws(buffer, bufferSize, fin);    //title
            oneNews.title = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.title, buffer + 3);  //omit "@T:"
            fgetws(buffer, bufferSize, fin);    //omit "@B:"
            fgetws(buffer, bufferSize, fin);    //main context
            oneNews.context = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(buffer) + 1));
            formatLine(buffer);
            wcscpy(oneNews.context, buffer);

            int cnt = 0;
            cnt = tokenize(&sentences, oneNews.context); // 這裡做斷句
            free(oneNews.context); oneNews.context=NULL;
            for (size_t i=0; i<cnt; ++i) {
                fwprintf(fout, L"%ls\t%ls\t%ls\n", sentences[i], oneNews.title, oneNews.url); // 寫入句子到檔案
                free(sentences[i]); sentences[i]=NULL;
            }

            free(oneNews.context); oneNews.context=NULL;
            free(oneNews.url); oneNews.url=NULL;
            free(oneNews.title); oneNews.title=NULL;
        }
        fclose(fin);

    }
    free(sentences);
    free(buffer);
    fclose(fout);
}

int main(const int argc, const char** argv)
{
    FILE* outFile = fopen("final.rec", "wb");
    setlocale(LC_CTYPE, "");
    parse();
    return 0;
}
