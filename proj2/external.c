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
const wchar_t *delim = L"，。！？!?\t\b\n";

uint expectedKeySize = 300;  //預設 300個中文字 //in char * 3 = 450
uint HashSize = 1000000; //預設 hash table size = 100000
unsigned long long memLimit = 500 * (1uLL<<20uLL); // default 100MB
////////////////////////////////////////////////////////////////////////////////
FILE* nodeFp;
// Knode 組成 node table, 會有 uint = 4G 個 Knode
typedef struct
{
    uint keyPos;    // 該 node 指向的 string table idx
    uint keyLen;
    uint cnt;       // string count
    uint next;      // 處理碰撞, 指向下一個 internal Knode
    long nextFilePos;   // 處理碰撞, 指向下一個 external Knode
}Knode;
// Hnode 組成 hash table，指向 hashvalue的第一個 Knode
typedef struct
{
    uint nodePos;   // internal
    long filePos;   // external
}Hnode;
// hash handler
typedef struct
{
    Hnode *hashTable;
    uint HashSize;
    Knode *nodeTable;
    uint KnodeSize, ndx;   //ndx指向尚未使用的knode
    wchar_t *keyBuf, *keyBufPtr;
    uint keyBufSized;
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
    nodeFp = fopen("exNode.rec", "wb+");
    handler->hashTable = (Hnode*)malloc(sizeof(Hnode)*HashSize);       //hash table 有 HashSize 個 hnode
    memLimit -= sizeof(uint)*HashSize;
    handler->HashSize = HashSize;
    handler->KnodeSize = (memLimit/10)/sizeof(Knode);
    //handler->KnodeSize = 10;
    handler->nodeTable = (Knode*)malloc(sizeof(Knode)*handler->KnodeSize);
    //handler->nodeTable = (Knode*)malloc(sizeof(Knode)*10);
    memLimit -= sizeof(Knode)*handler->KnodeSize;
    handler->ndx = 1;   // nodeTable[0] 為初始值
    handler->keyBufSized = memLimit/sizeof(wchar_t);
    //handler->keyBufSized = 1000000;
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

void readNode(hdbHandler* handler, uint hdx, long filePos, wchar_t* keySaved, Knode* tmpNode)
{
    if(hdx == 0)  // external node
	{
		fseek(nodeFp, filePos, SEEK_SET);
        fread(&tmpNode->keyPos, sizeof(unsigned int), 1, nodeFp);
        fread(&tmpNode->keyLen, sizeof(unsigned int), 1, nodeFp);
        fread(&tmpNode->cnt, sizeof(unsigned int), 1, nodeFp);
        tmpNode->next = 0;
        fread(&tmpNode->nextFilePos, sizeof(long), 1, nodeFp);
        //printf("%u %u %u %u %li\n", tmpKeyPos, tmpKeyLen, tmpCnt, tmpNext, tmpNextFilePos);
        wcsncpy(keySaved, handler->keyBuf + tmpNode->keyPos, expectedKeySize);
		return;
	}
    // internal node
    tmpNode->keyPos = handler->nodeTable[hdx].keyPos;
    tmpNode->keyLen = handler->nodeTable[hdx].keyLen;
    tmpNode->cnt = handler->nodeTable[hdx].cnt;
    tmpNode->next = handler->nodeTable[hdx].next;
	tmpNode->nextFilePos = handler->nodeTable[hdx].nextFilePos;
    wcsncpy(keySaved, handler->keyBuf + handler->nodeTable[hdx].keyPos, expectedKeySize);
	return;
}
void writeNode(hdbHandler* handler, uint hdx, long filePos, wchar_t* key, Knode* tmpNode)
{
    if(key != NULL)    //when key == NULL，做cnt++而已
    {
        wcscpy(handler->keyBufPtr, key);
        tmpNode->keyPos = (uint)(handler->keyBufPtr - handler->keyBuf);
        //printf("keyPos = %u\n", tmpNode->keyPos);
        tmpNode->keyLen = wcslen(key);
        handler->keyBufPtr += wcslen(key) + 1;
    }

    if(hdx == 0)  // external node
	{
		fseek(nodeFp, filePos, SEEK_SET);
        fwrite(&tmpNode->keyPos, sizeof(unsigned int), 1, nodeFp);
        fwrite(&tmpNode->keyLen, sizeof(unsigned int), 1, nodeFp);
        fwrite(&tmpNode->cnt, sizeof(unsigned int), 1, nodeFp);
        // external 不用紀錄 next(interal position)
        fwrite(&tmpNode->nextFilePos, sizeof(long), 1, nodeFp);
    	return;
	}// internal node
    handler->nodeTable[hdx].keyPos = tmpNode->keyPos;
    handler->nodeTable[hdx].keyLen = tmpNode->keyLen;
    handler->nodeTable[hdx].cnt = tmpNode->cnt;
    handler->nodeTable[hdx].next = tmpNode->next;
    handler->nodeTable[hdx].nextFilePos = tmpNode->nextFilePos;
    return;
}
// insert hash
void hdb_insert(hdbHandler *handler, wchar_t *str)
{
    uint hashValue = hash33(str) % handler->HashSize;
    uint hdx = handler->hashTable[hashValue].nodePos, previous = 0;    //when hdx == 0, 新的str
    long filePos = handler->hashTable[hashValue].filePos, previousFilePos = 0;  // external Knode

    Knode tmpNode = {0, 0, 0, 0, 0};
    while(hdx || filePos)  //when hdx != 0 || filePos != 0, existed same or collision
    {
        wchar_t* keySaved = (wchar_t*)malloc(sizeof(wchar_t)*expectedKeySize);  // read key from buf
        readNode(handler, hdx, filePos, keySaved, &tmpNode);
        if(!wcsncmp(str, keySaved, wcslen(str)))    // 找到相同的
		{
            free(keySaved); keySaved = NULL;
            tmpNode.cnt++;
			writeNode(handler, hdx, filePos, NULL, &tmpNode);
			return;
		}
        free(keySaved); keySaved = NULL;
		previous = hdx;
		hdx = tmpNode.next;    // when hdx == 0, insert new node
        previousFilePos = filePos;
        filePos = tmpNode.nextFilePos;
    }

    if(handler->keyBufSized < (uint)(handler->keyBufPtr - handler->keyBuf + wcslen(str)))
    {
        printf("realloc key buf\n");
        handler->keyBufSized <<= 1;
        uint lastPos = (uint)(handler->keyBufPtr - handler->keyBuf);
        handler->keyBuf = (wchar_t*)realloc(handler->keyBuf, handler->keyBufSized * sizeof(wchar_t));
        handler->keyBufPtr = handler->keyBuf + lastPos; //realloc後，將keyBufPtr指向新的最尾端
    }

    if(previous || previousFilePos)    // collision
    {
        // insert external node
        if(handler->ndx >= handler->KnodeSize)
        {
            // 檔案最尾
            fseek(nodeFp, 0, SEEK_END);
		    long retpos = ftell(nodeFp);
            // 將最後 collision 的 node 指向新的 node
            Knode collisionNode = {tmpNode.keyPos, tmpNode.keyLen, tmpNode.cnt, tmpNode.next, retpos};
            writeNode(handler, previous, previousFilePos, NULL, &collisionNode);
            // 插入新的node
            Knode newNode = {0, 0, 1, 0, 0};
            writeNode(handler, 0, retpos, str, &newNode);
            handler->ndx++;
            //printf("EX ndx = %u, nodeSize = %u, keyBufSized = %u , keyused = %u filePos = %li\n", handler->ndx, handler->KnodeSize, handler->keyBufSized, (uint)(handler->keyBufPtr - handler->keyBuf + wcslen(str)), retpos);
            return;
        }   // insert internal node
        else
        {
            // 將最後 collision 的 node 指向新的 node
            Knode collisionNode = {tmpNode.keyPos, tmpNode.keyLen, tmpNode.cnt, handler->ndx, 0};
            writeNode(handler, previous, 0, NULL, &collisionNode);
            Knode newNode = {0, 0, 1, 0, 0};
            writeNode(handler, handler->ndx++, 0, str, &newNode);
            return;
        }
    }
    else    // totally new
    {
        // insert external node
        if(handler->ndx >= handler->KnodeSize)
        {
            // 檔案最尾
            fseek(nodeFp, 0, SEEK_END);
		    long retpos = ftell(nodeFp);
            // 插入新的node
            Knode newNode = {0, 0, 1, 0, 0};
            writeNode(handler, 0, retpos, str, &newNode);
            handler->ndx++;
            handler->hashTable[hashValue].nodePos = 0;
            handler->hashTable[hashValue].filePos = retpos;
            //printf("EX ndx = %u, nodeSize = %u, keyBufSized = %u , keyused = %u filePos = %li\n", handler->ndx, handler->KnodeSize, handler->keyBufSized, (uint)(handler->keyBufPtr - handler->keyBuf + wcslen(str)), retpos);
            return;
        }// insert internal node
        else
        {
            handler->hashTable[hashValue].nodePos = handler->ndx;
            handler->hashTable[hashValue].filePos = 0;
            Knode newNode = {0, 0, 1, 0, 0};
            writeNode(handler, handler->ndx++, 0, str, &newNode);
            return;
        }
    }

    //printf("In ndx = %u, nodeSize = %u, keyBufSized = %u, keyused = %u\n", handler->ndx, handler->KnodeSize, handler->keyBufSized, (uint)(handler->keyBufPtr - handler->keyBuf + wcslen(str)));
    // 插入新的 internal node
    //Knode newNode = {0, 0, 1, 0, 0};
    //writeNode(handler, handler->ndx++, 0, str, &newNode);
    return;
}
////////////////////////////////////////////////////////////////////////////////
void setParameter(const int argc, const char **argv)
{
    /*
     * -h hash table size
     * -s expected key sized
     * -m memory limit
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
            HashSize = (uint)atoi(*(argv + i + 1));
            i += 1; continue;
        }
        else if(0 == strcmp( *(argv + i), "-m"))
        {
            memLimit = atoll(*(argv+i +1)) * (1uLL<<20uLL);
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

void formatLine(wchar_t *str)
{ // 假設輸入 str 只有一行內容
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
            formatLine(buffer);
            wchar_t *token=NULL, *ptr=NULL;
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
    //printf("keySized = %u HashSize = %u, memLimit = %llu\n", expectedKeySize, HashSize, memLimit);
    //printf("HashSize = %u KnodeSize = %u, keyBufSized = %u\n", handler->HashSize, handler->KnodeSize, handler->keyBufSized);
    parse(handler);
    printf("hash success!!!\n");

    qsort(handler->nodeTable, handler->KnodeSize, sizeof(Knode), cmp);
    printf("sorted!!! %d\n", handler->ndx);

    fseek(nodeFp, 0, SEEK_SET);
    Knode node = {0,0,0,0,0};
    long retpos = 0;
    wchar_t* readKey = (wchar_t*)malloc(sizeof(wchar_t)*expectedKeySize);
    // write internal node
    for (uint i = 1; i < handler->ndx; i++)
    {
        if(i < handler->KnodeSize)
        {   /*
            wchar_t* last = handler->keyBuf + handler->nodeTable[i].keyPos;
            //printf("i = %d, pos = %u, len = %u, cnt = %u\n%ls\n",i, handler->nodeTable[i].keyPos, handler->nodeTable[i].keyLen, handler->nodeTable[i].cnt, last);
            fwprintf(outFile, L"%u %ls\n", handler->nodeTable[i].cnt, last);
            continue;
            */
            readNode(handler, i, 0, readKey, &node);
            fwprintf(outFile, L"%u %ls\n", node.cnt, readKey);
            continue;
        }
        retpos = ftell(nodeFp);
        readNode(handler, 0, retpos, readKey, &node);
        fwprintf(outFile, L"%u %ls\n", node.cnt, readKey);
    }
    free(readKey);
    hdb_delete(handler);
    fclose (outFile);
    return 0;
}
