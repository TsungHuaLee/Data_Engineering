#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

typedef struct {
    size_t fileNum;
    char **file;
} splitFileHandler;

typedef struct {
    size_t n_record;
    char **data;
} Records;

const char *parameterPatterns[7] = {"-d", "-k", "-m", "-c", "-r", "-n", "-s"};
char *parameters[3] = {"@url", "NULL", "100"};  //-d, -k, -m
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
    char num[] = {'0' + i, '\0'};
    strcpy(fileName, tmpFilePrefix);   //filename path
    strcat(fileName, num);
    strcat(fileName, tmpFilePostfix);
    return;
}

void writeFile(FILE *fout, char **data, size_t recordCnt)
{
    for(size_t i = 0; i < recordCnt; i++)
    {
        fprintf(fout, "%s\n", data[i]);
        free(data[i]); data[i] = NULL;
    }
    return;
}

void splitAndSort()
{
    FILE* fp;
    fp = fopen("./data/total.rec", "rb");

    const unsigned long long dataLimit = atoll(parameters[2]) * (1uLL<<20uLL); // KB=2^10, MB=2^20
    unsigned long long recordCnt = 0, dataSize = 0, dataCap = 10000, bufferSize = 10000;
    char *oneLine = NULL, *buffer = NULL, **data = NULL, **tempPtr = NULL;    //buffer = one record, data = all record
    data = (char**)malloc(sizeof(char*)*dataCap);
    buffer = (char*)malloc(sizeof(char*)*bufferSize);
    unsigned long long bufferCnt = 0;
    oneLine = (char*)malloc(sizeof(char)*bufferSize);
    char fileName[16] = {};
    int fileNum = 0, eof = 0;

    while(1)
    {
        if(fgets(oneLine, bufferSize, fp)!=NULL)
            eof = 0;
        else
            eof = 1;
        // 當 buffer > 0 && 遇到下一個delimeter || 檔案結尾    --> 儲存一筆record
        if((strlen(buffer) > 0) && ((0 == strncmp(oneLine, parameters[0], delimeterLen)) || eof) )
        {
            if(recordCnt == dataCap)            //控制數量
            {
                dataCap *= 2;
                tempPtr = (char**)malloc(sizeof(char*)*dataCap);
                memcpy(tempPtr, data, sizeof(char*)*recordCnt);
                free(data);
                data = tempPtr;
                tempPtr = NULL;
            }
            dataSize += (unsigned long long)(sizeof(char)*(strlen(buffer)+1));   //calculate data size
            data[recordCnt] = (char*)malloc(sizeof(char)*(strlen(buffer)+10));
            memcpy(data[recordCnt], buffer, strlen(buffer) + 1);  //store one record
            memset(buffer, 0, strlen(buffer) + 1);      //reset buffer
            recordCnt++;
        }

        // write one split & sort file
        if(dataSize > dataLimit || eof)
        {
            qsort((void*)data, recordCnt, sizeof(char*), cmpfun);
            getFilePath(fileName, fileNum++);
            FILE *fout = fopen(fileName, "wb");
            writeFile(fout, data, recordCnt);
            // reset record, free recored data, malloc a new data
            recordCnt = 0;
            dataSize = 0;
            free(data);
            data = (char**)malloc(sizeof(char*)*dataCap);
        }
        // end of file
        if(eof)
            break;

        if(strlen(oneLine) == 2)
            continue;
        oneLine[strlen(oneLine)-1] = '\0';  //delete \n
        strcat(buffer, oneLine);
    }

    free(oneLine); oneLine = NULL;
    free(buffer); buffer = NULL;
    free(data); data = NULL;
    fclose(fp);
}


int cmpRecord(const Records *a, const Records *b) {
    /* precidition: a or b as data */
    if (a->n_record==0) return  1;
    if (b->n_record==0) return -1;
    return cmpfun(a->data, b->data);
}

void externalSort(splitFileHandler *handle)
{
    int front = 0, back = handle->fileNum, outfile = 0;
    char fileName[16];
    while(back - front) //when file num > 1
    {
        FILE *fin1 = fopen(handle->file[front++], "rb");
        FILE *fin2 = fopen(handle->file[front++], "rb");
        getFilePath(fileName, outfile);
        handle->file[handle->fileNum++] = fileName;
        back++;
        FILE *fout = fopen(fileName, "wb");
        size_t bufferSize = 10000, eof1 = 0, eof2 = 0;
        char *oneRecord = (char*)malloc(sizeof(char)*bufferSize), *oneRecord2 = (char*)malloc(sizeof(char)*bufferSize);
        Records file1, file2;
        file1.data = (char**)malloc(sizeof(char*)*2);
        file1.n_record = 0;
        file2.data = (char**)malloc(sizeof(char*)*2);
        file2.n_record = 0;

        if(fgets(oneRecord, bufferSize, fin1)!=NULL)
        {
            file1.data[0] = oneRecord;
            file1.n_record++;
        }
        else
            eof1 = 1;
        if(fgets(oneRecord2, bufferSize, fin2)!=NULL)
        {
            file2.data[0] = oneRecord2;
            file2.n_record++;
        }
        else
            eof2 = 1;


        while(1)
        {
            if(eof1 || eof2)
                break;
            int a = cmpRecord(&file1, &file2);
            //////////////////////////////////
            if(a > 0)
            {
                fprintf(fout, "%s", file1.data[0]);
                if(fgets(oneRecord, bufferSize, fin1)!=NULL)
                {
                    file1.data[0] = oneRecord;
                    file1.n_record++;
                }
                else
                    eof1 = 1;
            }
            if(a < 0)
            {
                fprintf(fout, "%s", file2.data[0]);
                if(fgets(oneRecord2, bufferSize, fin2)!=NULL)
                {
                    file2.data[0] = oneRecord2;
                    file2.n_record++;
                }
                else
                    eof2 = 1;
            }
            if(a==0)
            {
                fprintf(fout, "%s", file1.data[0]);
                fprintf(fout, "%s", file2.data[0]);
                if(fgets(oneRecord, bufferSize, fin1)!=NULL)
                {
                    file1.data[0] = oneRecord;
                    file1.n_record++;
                }
                else
                    eof1 = 1;
                if(fgets(oneRecord2, bufferSize, fin2)!=NULL)
                {
                    file2.data[0] = oneRecord2;
                    file2.n_record++;
                }
                else
                    eof2 = 1;
            }
            //////////////////////////////////////////
        }
        if(eof1)    //file1 complete, write file2
        {
            fprintf(fout, "%s", file2.data[0]);
            while(fgets(oneRecord2, bufferSize, fin2)!=NULL)
            {
                fprintf(fout, "%s", oneRecord2);
            }
        }
        if(eof2)    //file1 complete, write file2
        {
            fprintf(fout, "%s", file1.data[0]);
            while(fgets(oneRecord, bufferSize, fin1)!=NULL)
            {
                fprintf(fout, "%s", oneRecord);
            }
        }
        free(oneRecord);
        free(oneRecord2);
        fclose(fin1);
        fclose(fin2);
        fclose(fout);
    }
}

int main(const int argc, const char** argv)
{
    splitFileHandler handle;
    setParameter(argc, argv);
    splitAndSort();
    externalSort(&handle);
}
