#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
char recDelimeter;
char *keyPattern;
int keyPatExist, caseInsensitive, reverseOrder, numComparison;
const size_t bufferSize = 1000000;
const size_t chMinLen = 5;
const char *filePrefix = "./data/ettoday";
const size_t filePrefixLength = 14;
//const size_t fileNum = 6;
const size_t fileNum = 1;
const char *filePostfix = ".rec";

void setParameter(int argc, char ***argv)
{
    recDelimeter = '\n';
    keyPatExist = 0; caseInsensitive = 0; reverseOrder = 1; numComparison = 0;

    for(int i = 1; i < argc; i++)
    {
        if(0 == strcmp( *(*argv + i), "-d"))
        {
            //strcpy(recDelimeter, *(*argv + i + 1));
            recDelimeter = **(*argv + i + 1);
            i += 1; continue;
        }
        else if(0 == strcmp( *(*argv + i), "-k"))
        {
            keyPatExist = 1;
            keyPattern = (char*)malloc(sizeof(char) * (strlen(*(*argv + i + 1)) + 1) );
            strcpy(keyPattern, *(*argv + i + 1));
            i += 1; continue;
        }
        else if(0 == strcmp( *(*argv + i), "-c"))
        {
            caseInsensitive = 1;
            continue;
        }
        else if(0 == strcmp( *(*argv + i), "-r"))   //reverseOrder = -1 means reverse
        {
            reverseOrder = -1;
            continue;
        }
        else if(0 == strcmp( *(*argv + i), "-n"))
        {
            numComparison = 1;
            continue;
        }
    }
    return ;
}

void getFilePath(char* fileName, int i)
{
    char num[] = {'0' + i, '\0'};
    strcpy(fileName, filePrefix);   //filename path
    strcat(fileName, num);
    strcat(fileName, filePostfix);
}

int cmpfun(const void *a, const void *b)
{
    char **c = (char**)a;
    char **d = (char**)b;

    return reverseOrder*strcmp(*c, *d);
}

int ignoreCaseComp(const void *a, const void *b)
{
    char **c = (char**)a;
    char **d = (char**)b;

    int result, i = 0;
    while((result = toupper(*(*c + i)) - toupper(*(*d + i))) == 0)
    {
        i++;
        if(*(*c + i) == '\0')
            break;
    }
    return reverseOrder*result;
}

int read()
{
    char *buffer = NULL;
    char **sentences = NULL, **tempPtr = NULL;
    int strNum = 0;   //sentences count
    int capacity = 10000;  //sentences capacity
    FILE *fin;
    buffer = (char*)malloc(sizeof(char)*(bufferSize));  //about 1000000/3  chineses char
    sentences = (char**)malloc(sizeof(char*)*(capacity)); //capacity: 10000 sentences

    for(int i = 0; i < fileNum; i++)    //read all file
    {
        char fileName[16] = {};
        getFilePath(fileName, i);
        fin = fopen(fileName, "rb");    //open file
        if(fin == NULL) return -1;

        char oneChar;
        size_t bufflen = 0;
        while((oneChar = fgetc(fin))!=EOF)  //char by char
        {
            if(oneChar == recDelimeter)
            {
                if(strNum == capacity)
                {
                    capacity *= 2;
                    tempPtr = (char**)malloc(sizeof(char*)*capacity);
                    memcpy(tempPtr, sentences, sizeof(char*)*strNum);
                    free(sentences);
                    sentences = tempPtr;
                    tempPtr = NULL;
                }
                if(strlen(buffer) == 0)         //資料有些只有空行？？
                    continue;
                buffer[bufflen] = '\0';
                sentences[strNum] = (char*)malloc(sizeof(char) * strlen(buffer)+10);
                strcpy(sentences[strNum++], buffer);
                memset(buffer, 0, bufflen);
                bufflen = 0;
                continue;
            }
            if(oneChar == ' ' || oneChar == '\t')
                continue;
            *(buffer + bufflen++) = oneChar;
        }
        fclose(fin);
    }
    if(caseInsensitive)
        qsort((void*)sentences, strNum, sizeof(char*), ignoreCaseComp);
    else
        qsort((void*)sentences, strNum, sizeof(char*), cmpfun);

    FILE *fout = fopen("./insorted.txt", "w");
    printf("%d\n", strNum);
    for(int i = 0; i < strNum; i++)
    {
        fprintf(fout, "%s\n", sentences[i]);
        free(sentences[i]); sentences[i] = NULL;
    }
    free(sentences); sentences = NULL;
    free(buffer); buffer = NULL;
}

int main(int argc, char **argv)
{
    setParameter(argc, &argv);
    read();
}
