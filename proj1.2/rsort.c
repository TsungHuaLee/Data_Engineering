#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
typedef struct {
    size_t fileCnt;
    FILE **tmpFp;
} splitFileHandler;

typedef struct {
    size_t num, idx;
    char **data;
} Records;

const char *parameterPatterns[7] = {"-d", "-k", "-m", "-c", "-r", "-n", "-s"};
char *parameters[3] = {"@url", "NULL", "500"};  //-d, -k, -m
int setPara[4] ={0}, delimeterLen = 4; // -c, -r, -n, -s
const char *tmpFilePrefix = "./data/split";
const char *tmpFilePostfix = ".rec";

void setParameter(const int argc, const char **argv)
{
    /*
     * -d record_delimiter
     * -k key_pat
     * -m memory_limit (in MB)
     * -c case_insensitive
     * -r reverse order
     * -n numerical comparison
     * -s sort by size
    */
    for(int i = 1; i < argc; i++)
    {
        if(0 == strcmp( *(argv + i), "-d"))
        {
            parameters[0] = (char*)malloc(sizeof(char) * (strlen(*(argv + i + 1)) + 1) );
            strcpy(parameters[0], *(argv + i + 1));
            delimeterLen = strlen(parameters[0]);
            i += 1; continue;
        }
        else if(0 == strcmp( *(argv + i), "-k"))
        {
            parameters[1] = (char*)malloc(sizeof(char) * (strlen(*(argv + i + 1)) + 1) );
            strcpy(parameters[1], *(argv + i + 1));
            i += 1; continue;
        }
        else if(0 == strcmp( *(argv + i), "-m"))
        {
            parameters[2] = (char*)malloc(sizeof(char) * (strlen(*(argv + i + 1)) + 1) );
            strcpy(parameters[2], *(argv + i + 1));
            i += 1; continue;
        }
        else if(0 == strcmp( *(argv + i), "-c"))
        {
            setPara[0] = 1;
            continue;
        }
        else if(0 == strcmp( *(argv + i), "-r"))   //reverseOrder = -1 means reverse
        {
            setPara[1] = 1;
            continue;
        }
        else if(0 == strcmp( *(argv + i), "-n"))
        {
            setPara[2] = 1;
            continue;
        }
        else if(0 == strcmp( *(argv + i), "-s"))
        {
            setPara[3] = 1;
            continue;
        }
    }
    return ;
}

int cmpfun(const void *a, const void *b)
{
    const char *c = *(const char**)a;
    const char *d = *(const char**)b;
    int re = 0;
    if(setPara[3])      //-s, sort by size
    {
        re = strlen(c) - strlen(d);
        return setPara[1]? -re : re;
    }
    /* not robust enough. need to handle more exceptions */
    if(parameters[1] != NULL)   // -k, sort by key pattern
    {
        c = strstr(c, parameters[0]);
        if (c==NULL) return -1;
        d = strstr(d, parameters[0]);
        if (d==NULL) return  1;
    }
    if(setPara[2])          // -n, sort by numerical
    {
        int prechar = 0, numC = 0, numD = 0;
        while(c != NULL&& *c != '\0'&& !isdigit(*c))  //delete prefix character
        {
             c++;
             prechar++;
        }
        d += prechar;
        numC = atoi(c);
        numD = atoi(d);
        re = numC - numD;
    }
    else                    // sort by char
    {
        if(setPara[0])      // sort by ignore case charac
        {
            int i = 0;
            while((re = toupper(*(c + i)) - toupper(*(d + i))) == 0)
            {
                i++;
                if(*(c + i) == '\0' || *(d + i) == '\0')
                    break;
            }
        }
        else                //sort by case character
        {
            re = strcmp(c, d);
        }
    }
    return setPara[1]? -re : re;
}

void getFilePath(char* fileName, int i)
{
    char num[10];
    sprintf(num, "%d", i);
    strcpy(fileName, tmpFilePrefix);   //filename path
    strcat(fileName, num);
    strcat(fileName, tmpFilePostfix);
    return;
}

void writeOut(FILE *fout, char **data, size_t recordCnt)
{
    for(size_t i = 0; i < recordCnt; i++)
    {
        fprintf(fout, "%s\n", data[i]);
    }
    return;
}

void splitAndSort(splitFileHandler* handler)
{
    FILE* fp;
    fp = fopen("./data/total.rec", "rb");

    const unsigned long long dataLimit = atoll(parameters[2]) * (1uLL<<20uLL); // KB=2^10, MB=2^20
    unsigned long long recordCnt = 0, dataSize = 0, recordCap = 10000, bufferSize = 10000; // dataSize 紀錄單個檔案的總size, recordCap 紀錄單個檔案record數量
    char *oneLine = NULL, *buffer = NULL, **data = NULL, **tempPtr = NULL;    // buffer => one record, data => all record
    data = (char**)malloc(sizeof(char*)*recordCap);
    buffer = (char*)malloc(sizeof(char*)*bufferSize);
    unsigned long long bufferCnt = 0;
    oneLine = (char*)malloc(sizeof(char)*bufferSize);
    char fileName[16] = {};     // write out file name
    int fileCnt = 0, eof = 0, fileCap = 10;
    FILE **splitFp, **tempFp;   // all split file's pointer
    splitFp = (FILE**)malloc(sizeof(FILE*)*fileCap);
    while(1)
    {
        if(fgets(oneLine, bufferSize, fp)!=NULL)
            eof = 0;
        else
            eof = 1;
        // 當 buffer > 0 && 遇到下一個delimeter || 檔案結尾    --> 儲存一筆record
        if((strlen(buffer) > 0) && ((0 == strncmp(oneLine, parameters[0], delimeterLen)) || eof) )
        {
            if(recordCnt == recordCap)            //控制data數量
            {
                recordCap *= 2;
                tempPtr = (char**)malloc(sizeof(char*)*recordCap);
                memcpy(tempPtr, data, sizeof(char*)*recordCnt);
                free(data);
                data = tempPtr;
                tempPtr = NULL;
            }
            dataSize += (unsigned long long)(sizeof(char)*(strlen(buffer)+1));  //calculate data size
            data[recordCnt] = (char*)malloc(sizeof(char)*(strlen(buffer)+10));  // malloc for new record
            memcpy(data[recordCnt], buffer, strlen(buffer) + 1);  //store one record
            memset(buffer, 0, strlen(buffer) + 1);      //reset buffer
            recordCnt++;
        }

        // write one split & sort file
        if(dataSize > dataLimit || eof)
        {
            qsort((void*)data, recordCnt, sizeof(char*), cmpfun);   // internal sort
            getFilePath(fileName, fileCnt);
            FILE *fout = fopen(fileName, "wb+");
            writeOut(fout, data, recordCnt);
            if(fileCnt == fileCap)            //控制file pointer數量
            {
                fileCap *= 2;
                tempFp = (FILE**)malloc(sizeof(FILE*)*fileCap);
                memcpy(tempFp, splitFp, sizeof(FILE*)*fileCnt);
                free(splitFp);
                splitFp = tempFp;
                tempFp = NULL;
            }
            fseek(fout, 0, SEEK_SET);       // fp 指到 檔案開頭
            splitFp[fileCnt++] = fout;      // 存下 file pointer

            // reset record, free recored data, malloc a new data
            for(size_t i = 0; i < recordCnt; i++)
            {
                free(data[i]);
                data[i] = NULL;
            }
            recordCnt = 0;
            dataSize = 0;
        }
        // end of file
        if(eof)
            break;

        if(strlen(oneLine) == 2)    // omit @\n
            continue;
        oneLine[strlen(oneLine)-1] = '\0';  // delete \n, one record store in one line
        strcat(buffer, oneLine);
    }

    handler->fileCnt = fileCnt;
    handler->tmpFp = splitFp;
    free(oneLine); oneLine = NULL;
    free(buffer); buffer = NULL;
    free(data); data = NULL;
    fclose(fp);
}

int cmpRecord(const Records *a, const Records *b)
{
    /* precidition: a or b as data */
    if (a->num == 0) return  1;
    if (b->num == 0) return -1;
    return cmpfun(a->data, b->data);
}
// records is empty
void readRecord(FILE* fp, Records* records, size_t recordNum)
{
    records->data = (char**)malloc(sizeof(char*)*2);
    char *temp = (char*)malloc(sizeof(char)*10000);
    if(fgets(temp, 10000, fp) != NULL)
        records->num = recordNum;
    else
        records->num = 0;
    records->data[0] = temp;    //if eof, records->num will be 0, data will be NULL
    temp = NULL;
    return;
}

void cleanRecordData(Records *records)
{
    for(int i = 0; i < records->num; i++)
        free(records->data[i]);
    free(records->data);
}

void externalSort(splitFileHandler *handler)
{
#define LCH(X) ((X<<1)+1)
#define RCH(X) ((X<<1)+2)
#define PAR(X) ((X-1)>>1)
    FILE *fout = fopen("final.rec", "wb");
    size_t nodeNum = 0, *nodes;  // node的值 為 第幾個file在這個node
    Records *records = NULL;     // 紀錄每一個file 的 record
    nodeNum = 2 * handler->fileCnt; // node number = double file number
    nodes = (size_t*)malloc(sizeof(size_t)*(nodeNum));   // root is 0
    memset(nodes, 0xFF, sizeof(size_t)*nodeNum);
    records = (Records*)malloc(sizeof(Records)*(handler->fileCnt + 1));
    /* read one record from fp */
    for(int i = 0; i < handler->fileCnt; i++)
        readRecord(handler->tmpFp[i], records + i, 1);
    /* initialize leaf node
        first leaf: fileCnt - 1
        last leaf: 2(fileCnt - 1) */
    for(int i = handler->fileCnt - 1, j = 0; j < handler->fileCnt; i++, j++)
    {
        nodes[i] = j;    //leaf node 紀錄 第幾個fp
    }
    /* initialize tree */
    for(int i = handler->fileCnt - 2; i >= 0; i--)
    {
        int lch = LCH(i), rch = RCH(i);
        int cmp = cmpRecord(records + nodes[lch], records + nodes[rch]);
        nodes[i] = (cmp <= 0)?nodes[lch]:nodes[rch];   // 小的放前面
        //printf("%d %d %lu %lu\n%lu %lu\n %d %d %lu\n\n", lch, rch, nodes[lch], nodes[rch], strlen(records[nodes[lch]].data[0]), strlen(records[nodes[rch]].data[0]), cmp, i, nodes[i]);
    }

    while(1)
    {
        // while root is not NULL
        if(records[nodes[0]].num == 0) break;
        writeOut(fout, records[nodes[0]].data, 1);
        cleanRecordData((records + nodes[0]));
        readRecord(handler->tmpFp[nodes[0]], records + nodes[0], 1);   // update leaf node
        // adjust tree by buttom up
        for(int par = PAR(handler->fileCnt - 1 + nodes[0]); par >= 0; par = PAR(par))
        {
            int lch = LCH(par), rch = RCH(par);
            int cmp = cmpRecord(records + nodes[lch], records + nodes[rch]);
            //update parent
            nodes[par] = (cmp <= 0)?nodes[lch]:nodes[rch];   // 小的放前面
        }
    }

    free(nodes);  nodes = NULL;
    for (int i = 0; i < handler->fileCnt; ++i) cleanRecordData(records+i);
    free(records); records = NULL;
    fclose(fout); fout = NULL;
}

void cleanFileHandler(splitFileHandler *handler)
{
    for(int i = 0; i < handler->fileCnt; i++)
    {
        free(handler->tmpFp[i]);
        handler->tmpFp[i] = NULL;
    }
    free(handler->tmpFp); handler->tmpFp = NULL;
}
int main(const int argc, const char** argv)
{
    splitFileHandler handler;
    setParameter(argc, argv);
    time_t start, end;
	start = time(NULL);
    splitAndSort(&handler);
    end = time(NULL);
	printf("execute time = %ld\n", end-start);

    start = time(NULL);
    externalSort(&handler);
    end = time(NULL);
	printf("execute time = %ld\n", end-start);
    cleanFileHandler(&handler);
}
