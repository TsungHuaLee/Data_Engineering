#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
const size_t chMinLen = 5;
const char* files[] = {"./data/ettoday0.rec", "./data/ettoday1.rec", "./data/ettoday2.rec", "./data/ettoday3.rec", "./data/ettoday4.rec", "./data/ettoday5.rec"};
const size_t fileNum = 6;
// 中文 Unicode 範圍
const unsigned long minChinese = 0x4E00;
const unsigned long maxChinese = 0x9FFF;
//判斷斷句符號
const wchar_t *delim = L"，。！？!?\n";

size_t expectedKeySize = 300;  //預設 300個中文字 //in char * 3 = 450
uint maxHash = 1000000; //預設 hash table size = 1000000

////////////////////////////////////////////////////////////////////////////////
// Knode 組成 node table, 會有 uint = 4G 個 Knode
typedef struct
{
    uint keyPos;    // 該 node 指向的 string table idx
    uint keyLen;
    uint cnt;       // string count
    uint next;      // 處理碰撞, 指向下一個Knode
}Knode;
// hash handler
typedef struct
{
    uint *hashTable;
    size_t maxHash;
    Knode *nodeTable;
    size_t KnodeSize, ndx;   //ndx指向尚未使用的knode
    wchar_t *keyBuf, *keyBufPtr;
    size_t keyBufSized;
}hdbHandler;
// hash function
unsigned long hash33(wchar_t *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
// initialize hash
void hdb_init(hdbHandler *handler)
{
    handler->hashTable = (uint*)malloc(sizeof(uint)*maxHash);       //hash table 有 maxhash 個 maxnode
    memset(handler->hashTable, 0, maxHash * sizeof(uint));
    handler->maxHash = maxHash;
    handler->KnodeSize = 1000000;
    handler->nodeTable = (Knode*)malloc(sizeof(Knode)*handler->KnodeSize);
    handler->ndx = 1;   // nodeTable[0] 為初始值
    handler->keyBufSized = 1000000;
    handler->keyBuf = (wchar_t*)malloc(sizeof(wchar_t)*(handler->keyBufSized));
    handler->keyBufPtr = handler->keyBuf;       //keyBufPtr 指向 keyBuf 的結尾
}
// delete hash
void hdb_delete(hdbHandler *handler)
{
    free(handler->hashTable); handler->hashTable = NULL;
    handler->keyBufPtr = NULL;
    free(handler->keyBuf); handler->keyBuf = NULL;
}
// insert hash
void hdb_insert(hdbHandler *handler, wchar_t *str)
{
    uint hashValue = hash33(str) % handler->maxHash;
    uint hdx = handler->hashTable[hashValue], previous = 0;    //when hdx == 0, 新的str
    while(hdx)  //when hdx != 0, existed same or collision
    {
        if(!wcsncmp(str, handler->keyBuf + handler->nodeTable[hdx].keyPos, wcslen(str)))    // 找到相同的
		{
			handler->nodeTable[hdx].cnt++;
			return;
		}
		previous = hdx;
		hdx = handler->nodeTable[hdx].next;    // when hdx == 0, insert new node
    }
    if(previous)    // collision
        handler->nodeTable[previous].next = handler->ndx;
    else            // totally new
        handler->hashTable[hashValue] = handler->ndx;

    if(handler->ndx == handler->KnodeSize)
	{
		handler->KnodeSize <<= 1;
		handler->nodeTable = (Knode *)realloc(handler->nodeTable, handler->KnodeSize * sizeof(Knode));
	}

    if(handler->keyBufSized < (handler->keyBufPtr - handler->keyBuf + wcslen(str)))
    {
        handler->keyBufSized <<= 1;
        uint lastPos = handler->keyBufPtr - handler->keyBuf;
        handler->keyBuf = (wchar_t*)realloc(handler->keyBuf, handler->keyBufSized * sizeof(wchar_t));
        handler->keyBufPtr = handler->keyBuf + lastPos;
    }
    //printf("ndx = %lu, nodeSize = %lu, keyBufSized = %lu , keyused = %lu , new = %lu\n", handler->ndx, handler->KnodeSize, handler->keyBufSized, handler->keyBufPtr - handler->keyBuf + wcslen(str), wcslen(str));
    // insert new node
    wcscpy(handler->keyBufPtr, str);
    handler->nodeTable[handler->ndx].keyPos = (uint)(handler->keyBufPtr - handler->keyBuf);
    handler->nodeTable[handler->ndx].keyLen = wcslen(str);
    handler->keyBufPtr += wcslen(str) + 1;
    handler->nodeTable[handler->ndx].cnt = 1;
    handler->nodeTable[handler->ndx++].next = 0;
    return;
}
////////////////////////////////////////////////////////////////////////////////
void setParameter(const int argc, const char **argv)
{
    /*
     * -h hash table size
     * -s expected key sized
    */
    for(int i = 1; i < argc; i++)
    {
        if(0 == strcmp( *(argv + i), "-s"))
        {
            expectedKeySize = (uint)atoi(*(argv + i + 1));
            i += 1; continue;
        }
        else if(0 == strcmp( *(argv + i), "-h"))
        {
            maxHash = (uint)atoi(*(argv + i + 1));
            i += 1; continue;
        }
    }
    return ;
}
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

void parse(hdbHandler *handler)
{
    wchar_t *buffer = NULL;
    FILE *fin;
    for(int i = 0; i < 1; i++)
    {
        if(i > 0)
            break;

        fin = fopen(files[i], "rb");    //open file
        if(fin == NULL)
        {
            printf("file%d is not existed\n", i);
            return;
        }
        buffer = (wchar_t*)malloc(sizeof(wchar_t)*(10000+10));
        while(fgetws(buffer, expectedKeySize, fin) != NULL)
        {
            // tokenize
            if (wcsncmp (buffer, L"@", 1) == 0)
					continue;
            wchar_t *token=NULL, *ptr=NULL;
            int cnt = 0;
            token = wcstok(buffer, delim, &ptr);
            while(token!=NULL)
            {
                hdb_insert(handler, token);
                token = wcstok(NULL, delim, &ptr);
            }
            //end tokenize
        }
        fclose(fin);
    }
    free(buffer);
    return ;
}

int cmp (const void * a, const void * b)
{
	if ((*(Knode *)a).cnt < (*(Knode *)b).cnt)
		return 1;
	return 0;
}

int main(const int argc, const char** argv)
{
    setlocale(LC_CTYPE, "");
    FILE* outFile = fopen("final.rec", "wb+");
    hdbHandler *handler = (hdbHandler*)malloc(sizeof(hdbHandler)*1);
    setParameter(argc, argv);
    hdb_init(handler);
    parse(handler);
    printf("hash success!!!\n");
    qsort(handler->nodeTable, handler->ndx, sizeof(Knode), cmp);
    printf("sorted!!!, %lu\n", handler->ndx);

    for (uint i = 1; i < handler->ndx; i++)
    {
        wchar_t* last = handler->keyBuf + handler->nodeTable[i].keyPos;
        //printf("i = %d, pos = %u, len = %u, cnt = %u\n%ls\n",i, handler->nodeTable[i].keyPos, handler->nodeTable[i].keyLen, handler->nodeTable[i].cnt, last);
        fwprintf(outFile, L"%u %ls\n", handler->nodeTable[i].cnt, last);
    }

    hdb_delete(handler);
    fclose (outFile);
    return 0;
}
