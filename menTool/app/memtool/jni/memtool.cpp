
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include "stdarg.h"
#include <initializer_list>
#include <vector>
#include <jni.h>
//1.定义搜索范围
//2.定义搜索方式-偏移/类似gg范围/mprotect直接直接注入式搜索
//3.确定要搜索的数值类型
//4.确定要搜索的数值



using namespace std;

struct MemoryAddressRange {
    long int Begin_address;
    long int End_address;
    struct MemoryAddressRange *next;
};

struct MemoryAddressFromSearch {
    long int Address;
    struct MemoryAddressFromSearch *next;
};


#define MemoryAddress_Size sizeof(struct MemoryAddressRange)
#define MemoryAddressFromSearch_Size sizeof(struct MemoryAddressFromSearch)
#define PageNum 33


typedef struct MemoryAddressRange *MAR;
typedef struct MemoryAddressFromSearch *MAFS;
//c++不用unsigned，因为gg中可以搜索负数
typedef char BYTE;
typedef short WORD;

typedef long DWORD;


int buff[1024];
float buff1[1024];
double buff2[1024];
WORD buff3[1024];
BYTE buff4[1024];




//---------------------------------------------函数声明-----------------------------------------------------

//-----------------------------1.Tools区-------------------------------

//打印文字
void print(string a) {
    cout << a << endl;
}

//char转换double
double sqrt(double sum, int i);

double hexToDec(char *str);

//用来释放链表中申请的内存空间
void ClearMyList(MAR *ppHeadNode) {
    MAR pListNodeTmp = NULL;
    if ((*ppHeadNode) == NULL) {
        print("The list is empty, no need to clear.\n");
        return;
    }

    // 循环释放链表中的结点所占内存，清空结束后
    while ((*ppHeadNode)->next != NULL) {
        pListNodeTmp = (*ppHeadNode)->next;
        free((*ppHeadNode));
        (*ppHeadNode) = pListNodeTmp;
    }

    // 清除最后一个结点
    if ((*ppHeadNode) != NULL) {
        free((*ppHeadNode));
        (*ppHeadNode) = NULL;
    }
    print("The list is cleared.\n");
}

//获得链表节点
int GetListNodeLen(MAFS m_pHead) {
    if (m_pHead == NULL) {
        return 0;
    }

    MAFS pTemp = m_pHead;
    int aListLen = 0;
    while (pTemp != NULL)    //判断当前节点指针是否为空
    {
        aListLen++;
        pTemp = pTemp->next;
    }
    return aListLen;
}

//打印链表--会自动清除链表
void Print_Linked_list_MAR(MAR HEAD) {
    MAR pointer = HEAD;
    while (pointer) {
        cout << (*pointer).Begin_address << ":::" << (*pointer).End_address << endl;
        pointer = (*pointer).next;
    }
    ClearMyList((MAR *) pointer);
    free(pointer);
}

//打印链表--会自动清除链表
void Print_Linked_list_MAFS(MAFS HEAD) {
    MAFS pointer = HEAD;
    while (pointer) {
        cout << (*pointer).Address << endl;
        pointer = (*pointer).next;
    }
    //ClearMyList((MAR *) pointer);
    free(pointer);
}

//String转换char类型
void String_To_Char(char valueChar[], const string &value);

//获取除去最后一位的前面的数值
string SplitString_CutLastcChar(char YourString[]);

//根据~分割
string mySplit_Result(char str[], int flag);

//链表合并:合并la、lb到lc
void MergeLinkList_LL(MAFS &LA, MAFS &LB, MAFS &LC);


//-----------------------------2.功能区-------------------------------

//获取包名对应得PID（需要root权限）
int getPID(const char *packageName);

//根据PID停止某个应用
int StopPID(int pid);

//根据PID恢复被停止得应用
int ResetFromStopPID(int pid);

//-----------------------------3.内存修改-------------------------------
//设置内存范围
MAR SetMemorySearchRange(int type, int pid);

//----------------------联合搜索-------------------------------

//联合搜索（首次）
//传入搜索得value数值，要修改得范围类型，valueType
MAFS UnionSearch_First(int Rangetype, int pid, string value, int ValueType);

//联合搜索（二次~N次过滤）
MAFS UnionSearch_Filter_value(MAFS Head, int pid, string value, int ValueType);

MAFS UnionSearch_Filter_Rangevalue(MAFS Head, int pid, string value, int ValueType);



//---------------------------------------------内存范围设定-----------------------------------------------------

//----确定搜索的内存范围
//type:分别对应数字：0-11
//    all:          所有内存
//    B_BAD         B/v内存
//    V             v内存
//    C_ALLOC       CA内存
//    C_BSS         CB内存
//    C_DATA        CD内存
//    C_HEAP        CH内存
//    JAVA_HEAP     JH内存
//    A_ANONMYOURS  A内存
//    CODE_SYSTEM   Xs内存
//    STACK         S内存
//    ASHMEM        AS内存

//所有的话搜索太慢了，几分钟都出不来结果！后期考虑去除
MAR Read_Maps_Ruturn_MAR_all(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL)//strstr:字符匹配
        {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_B(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "/system/fonts") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_V(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "/dev/kgsl-3d0") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_Ca(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "[anon:libc_malloc]") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_Cb(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "[anon:.bss]") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_Cd(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "/data/") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_Ch(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "[heap]") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_jh(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "/dev/ashmem/") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_A(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strlen(buff) < 42) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_Xs(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "/system") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_S(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "[stack]") != NULL) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

MAR Read_Maps_Ruturn_MAR_As(int pid) {
    MAR HEAD = nullptr;
    MAR Begin = nullptr;
    MAR End = nullptr;
    HEAD = Begin = End = (MAR) malloc(MemoryAddress_Size);

    FILE *fp;
    int i = 0, flag = 1;
    char lj[64], buff[256];

    sprintf(lj, "/proc/%d/maps", pid);
    fp = fopen(lj, "r");
    if (fp == NULL) {
        puts("分析失败");
        return NULL;
    }
    while (!feof(fp)) {
        fgets(buff, sizeof(buff), fp);//读取一行
        if (strstr(buff, "rw") != NULL && strstr(buff, "/dev/ashmem/") != NULL &&
            !strstr(buff, "dalvik")) {
            sscanf(buff, "%lx-%lx", &End->Begin_address, &End->End_address);
            //这里使用lx是为了能成功读取特别长的地址
            flag = 1;
        } else {
            flag = 0;
        }
        if (flag == 1) {
            i++;
            if (i == 1) {
                End->next = nullptr;
                Begin = End;
                HEAD = Begin;
            } else {
                End->next = nullptr;
                Begin->next = End;
                Begin = End;
            }
            End = (MAR) malloc(MemoryAddress_Size);//不重新分配会导致链表无法串起
        }
    }
    free(End);//将多余的空间释放
    fclose(fp);//关闭文件指针
    return HEAD;
}

//返回的是一个存储了各个内存范围的maps链表
MAR SetMemorySearchRange(int type, int pid) {
    MAR HEAD = nullptr;

    switch (type) {
        case 0:
            HEAD = Read_Maps_Ruturn_MAR_all(pid);
            return HEAD;
        case 1:
            HEAD = Read_Maps_Ruturn_MAR_B(pid);
            return HEAD;
        case 2:
            HEAD = Read_Maps_Ruturn_MAR_V(pid);
            return HEAD;
        case 3:
            HEAD = Read_Maps_Ruturn_MAR_Ca(pid);
            return HEAD;
        case 4:
            HEAD = Read_Maps_Ruturn_MAR_Cb(pid);
            return HEAD;
        case 5:
            HEAD = Read_Maps_Ruturn_MAR_Cd(pid);
            return HEAD;
        case 6:
            HEAD = Read_Maps_Ruturn_MAR_Ch(pid);
            return HEAD;
        case 7:
            HEAD = Read_Maps_Ruturn_MAR_jh(pid);
            return HEAD;
        case 8:
            HEAD = Read_Maps_Ruturn_MAR_A(pid);
            return HEAD;
        case 9:
            HEAD = Read_Maps_Ruturn_MAR_Xs(pid);
            return HEAD;
        case 10:
            HEAD = Read_Maps_Ruturn_MAR_S(pid);
            return HEAD;
        case 11:
            HEAD = Read_Maps_Ruturn_MAR_As(pid);
            return HEAD;

    }

    return HEAD;
}

//--------------------------------------------------------------------------------------------------


//获取进程pid需要在linux系统下，Clion下不测试了
int getPID(const char *packageName) {
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    FILE *fp = NULL;
    char filepath[256];            // 大小随意，能装下cmdline文件的路径即可
    char filetext[128];            // 大小随意，能装下要识别的命令行文本即可
    dir = opendir("/proc");        // 打开路径
    if (NULL != dir) {
        while ((ptr = readdir(dir)) != NULL)    // 循环读取路径下的每一个文件/文件夹
        {
            // 如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
            if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                continue;
            if (ptr->d_type != DT_DIR)
                continue;
            sprintf(filepath, "/proc/%s/cmdline", ptr->d_name);    // 生成要读取的文件的路径
            fp = fopen(filepath, "r");    // 打开文件
            if (NULL != fp) {
                fgets(filetext, sizeof(filetext), fp);    // 读取文件
                if (strcmp(filetext, packageName) == 0) {
                    //puts(filepath);
                    //printf("packagename:%s\n",filetext);
                    break;
                }
                fclose(fp);
            }
        }
    }
    if (readdir(dir) == NULL) {
        //puts("Get pid fail");
        return 0;
    }
    closedir(dir);    // 关闭路径
    return atoi(ptr->d_name);
}

//--------------------------------------------------------------------------------------------------


//联合搜索
//这种方法已经放弃
void FirstSearch(std::initializer_list <std::string> il) {
//    c_str:string转char
    for (auto s:il) {

        if (strstr(s.c_str(), "D") != NULL) {
            cout << "your search num is DWORD" << endl;
        } else if (strstr(s.c_str(), "F") != NULL) {
            cout << "your search num is float" << endl;
        } else if (strstr(s.c_str(), "E") != NULL) {
            cout << "your search num is double" << endl;
        } else if (strstr(s.c_str(), "W") != NULL) {
            cout << "your search num is Word" << endl;
        } else if (strstr(s.c_str(), "B") != NULL) {
            cout << "your search num is Byte" << endl;
        } else if (strstr(s.c_str(), "~") != NULL) {
            cout << "your search num is RangeValue" << endl;
        } else {
            cout << "please Input value_style" << endl;
        }
//        cout << s << endl;
    }
}

//--------------------------------------------------------------------------------------------------

//分割范围搜索的范围数值
void mySplit(const string &inStr, vector <string> &outStr, char *split) {
    /*此代码之下为切割*/
    //char *split = "+-";//23+86-6+37+24-8-13

    //string转char *。c_str()是string转const char *
    char *tempStr = new char[inStr.size()];
    //#pragma warning(disable:4996)
    inStr.copy(tempStr, inStr.size(), 0); //这里5，代表复制几个字符，0代表复制的位置
    *(tempStr + inStr.size()) = '\0'; //要手动加上结束符

    char *p2 = strtok(tempStr, split);
    while (p2 != NULL) {
        outStr.push_back(p2);
        //cout << p2 << endl;
        p2 = strtok(NULL, split);
    }
    /*此代码之上为切割*/
}

//根据~分割
string mySplit_Result(char str[], int flag) {
    string str_new = str;
    string str1 = "~";
    vector <string> outStr;
    char *chr1 = const_cast<char *>(str1.c_str());
    mySplit(str, outStr, chr1);
    if (flag == 0) {
        return outStr[0];

    } else if (flag == 1) {
        return outStr[1];
    } else {
        return NULL;
    }


}

//获取除去最后一位的前面的数值
string SplitString_CutLastcChar(char YourString[]) {
    int len = strlen(YourString);
    string str = YourString;
    string newstr = str.substr(0, len - 1);
    return newstr;
}



//--------------------------------------------------------------------------------------------------

//暂停某个应用
int StopPID(int pid) {
    char ml[64];
    sprintf(ml, "kill -STOP %d", pid);
    if (getuid() != 0) {
        system(ml);
        exit(1);//退出没有root的进程
    }
}

//恢复某个应用
int ResetFromStopPID(int pid) {
    char ml[64];
    sprintf(ml, "kill -CONT %d", pid);
    if (getuid() != 0) {
        system(ml);
        exit(1);//退出没有root的进程
    }
}


//-----------------------------------------------链表合并---------------------------------------------------
//链表合并:合并la、lb到lc
void MergeLinkList_LL(MAFS &LA, MAFS &LB, MAFS &LC) {
    MAFS pa, pb, pc;        // 定义三个链表
    pa = LA->next;                // pa指向LA指针
    pb = LB->next;                // pb指向LB指针
    LC = LA;                    // 用La的头结点作为LC头结点
    pc = LC;                    // pc的初值指向LC的头结点

    while (pa && pb) {            //链表不为空
        if (pa->Address <= pb->Address)    //先放小的数据
        {
            pc->next = pa;        //pc的next指向pa
            pc = pa;            //pc指向pa
            pa = pa->next;        //pa指向pa的next
        } else {
            pc->next = pb;        //pc的next指向pb
            pc = pb;            //pc指向pb
            pb = pb->next;        //pa指向pb的next
        }
    }
    pc->next = pa ? pa : pb;    // 插入非空表的剩余段
    delete LB;
}

//本人clion编译器版本会莫名其妙错误输出最后一个值
MAFS add(MAFS heada, MAFS p0) {
    MAFS p1, p2;
    p1 = heada;
    while (/*p1->next!=NULL*/p1 != NULL && p1->Address < p0->Address) {
        p2 = p1;
        p1 = p1->next;
    }
    if (p1 == heada) {
        heada = p0;
        p0->next = p1;
    } else {
        if (p2->next != NULL) {
//            p2->next = p0;
//            p0->next = p1;    /*顺序不要搞错*/
            p0->next = p1;
            p2->next = p0;
        } else {
            p2->next = p0;
            p0->next = NULL;
        }
    }
    return heada;
}

MAFS mergeTwoListsV2(MAFS TEST1, MAFS TEST2) {
    MAFS p0, p;
    do {
        p0 = (MAFS) malloc(sizeof(MemoryAddressFromSearch_Size));
        p0->Address = TEST2->Address;
        p = add(TEST1, p0);
        TEST2 = TEST2->next;
    } while (TEST2);
    return p;
}


//-----------------------------------------------内存搜索---------------------------------------------------
//1.联合搜索
//ValueType:
//          0.Dword-4字节
//          1.float-4字节
//          2.Double-8字节
//          3.Word-2字节
//          4.Byte-1字节
MAFS UnionSearch_First(int Rangetype, int pid, string value, int ValueType) {
    MAR Head = SetMemorySearchRange(Rangetype, pid);
    MAFS Address_From_FirstSearch_End = nullptr;
    MAFS Address_From_FirstSearch_Begin = nullptr;
    MAFS Address_From_FirstSearch_Head = nullptr;
    Address_From_FirstSearch_Head = Address_From_FirstSearch_Begin = Address_From_FirstSearch_End = (MAFS) malloc(
            MemoryAddressFromSearch_Size);
    char valueChar[] = "";
    String_To_Char(valueChar, value);
    int num_count = 0;

    if (strstr(valueChar, "~")) {
        char value_From[] = "";
        char value_To[] = "";

        string From_Value = mySplit_Result(valueChar, 0);
        string To_Value = mySplit_Result(valueChar, 1);

        String_To_Char(value_From, From_Value);
        String_To_Char(value_To, To_Value);

        int int_value_real_From, int_value_real_To;
        float float_value_real_from, float_value_real_to;
        double double_value_real_from, double_value_real_to;
        WORD WORD_value_real_from, WORD_value_real_to;//需要转换int再转换char
        BYTE BYTE_value_real_from, BYTE_value_real_to;//需要转化int再转换short

        switch (ValueType) {
            case 0:
                int_value_real_From = atoi(value_From);
                int_value_real_To = atoi(value_To);

                break;
            case 1:
                float_value_real_from = atof(value_From);
                float_value_real_to = atof(value_To);
                break;
            case 2:
                double_value_real_from = hexToDec(value_From);
                double_value_real_to = hexToDec(value_To);
                break;
            case 3:
                WORD_value_real_from = (WORD) atoi(value_From);
                WORD_value_real_to = (WORD) atoi(value_To);
                break;
            case 4:
                BYTE_value_real_from = (BYTE) atoi(value_From);
                BYTE_value_real_to = (BYTE) atoi(value_To);
                break;

        }

        char lj[128];
        sprintf(lj, "/proc/%d/mem", pid);
        int handle = open(lj, 00000002);


        if (ValueType == 0) {
            buff[1024] = {0};

        } else if (ValueType == 1) {
            buff1[1024] = {0};

        } else if (ValueType == 2) {
            buff2[1024] = {0};

        } else if (ValueType == 3) {
            buff3[1024] = {0};

        } else if (ValueType == 4) {
            buff4[1024] = {0};

        }


        //pread=read+lseek,所以其实可以不用lseek
        //clion中当前编译器的linux内核版本不支持pread64

        MAR pointer = Head;
        int IsRight_address = 0;

        printf("BeginSearchValue");
        while (pointer) {
            for (long int i = (*pointer).Begin_address; i <= (*pointer).End_address; i = i + 0x4) {
                switch (ValueType) {
                    case 0:
                        memset(buff, 0, 4);
                        pread64(handle, buff, 4, i);
                        if (buff[0] >= int_value_real_From && buff[0] <= int_value_real_To) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 1:
                        memset(buff1, 0, 4);
                        pread64(handle, buff1, 4, i);
                        if (buff1[0] >= float_value_real_from && buff1[0] <= float_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 2:
                        memset(buff2, 0, 8);
                        pread64(handle, buff2, 8, i);
                        if (buff2[0] >= double_value_real_from &&
                            buff2[0] <= double_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 3:
                        memset(buff3, 0, 2);
                        pread64(handle, buff3, 2, i);
                        if (buff3[0] >= WORD_value_real_from && buff3[0] <= WORD_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 4:
                        memset(buff4, 0, 1);
                        pread64(handle, buff4, 1, i);
                        if (buff4[0] >= BYTE_value_real_from && buff4[0] <= BYTE_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                }
                if (IsRight_address == 1) {
                    Address_From_FirstSearch_End->Address = i;
                    print("address:");
                    cout << i << endl;
                    if (num_count == 0) {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Head = Address_From_FirstSearch_Begin;
                    } else {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin->next = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                    }
                    Address_From_FirstSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

                }

            }
            pointer = (*pointer).next;

        }
        close(handle);
        free(Address_From_FirstSearch_End);
        return Address_From_FirstSearch_Head;


    } else {
        char value_[] = "";


        String_To_Char(value_, value);


        int int_value_real;
        float float_value_real;
        double double_value_real;
        WORD WORD_value_real;//需要转换int再转换char
        BYTE BYTE_value_real;//需要转化int再转换short

        switch (ValueType) {
            case 0:
                int_value_real = atoi(value_);
                break;
            case 1:
                float_value_real = atof(value_);
                break;
            case 2:
                double_value_real = hexToDec(value_);
                break;
            case 3:
                WORD_value_real = (WORD) atoi(value_);
                break;
            case 4:
                BYTE_value_real = (BYTE) atoi(value_);
                break;

        }

        char lj[128];
        sprintf(lj, "/proc/%d/mem", pid);
        int handle = open(lj, 00000002);


        if (ValueType == 0) {
            buff[1024] = {0};

        } else if (ValueType == 1) {
            buff1[1024] = {0};

        } else if (ValueType == 2) {
            buff2[1024] = {0};

        } else if (ValueType == 3) {
            buff3[1024] = {0};

        } else if (ValueType == 4) {
            buff4[1024] = {0};

        }


        //pread=read+lseek,所以其实可以不用lseek
        //clion中当前编译器的linux内核版本不支持pread64

        MAR pointer = Head;
        int IsRight_address = 0;

//        int int_value_real;
//        float float_value_real;
//        double double_value_real;
//        WORD WORD_value_real;//需要转换int再转换char
//        BYTE BYTE_value_real;//需要转化int再转换short
        while (pointer) {
            for (long int i = (*pointer).Begin_address; i <= (*pointer).End_address; i = i + 0x4) {
                switch (ValueType) {
                    case 0:
                        memset(buff, 0, 4);
                        pread64(handle, buff, 4, i);
                        if (buff[0] == int_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 1:
                        memset(buff1, 0, 4);
                        pread64(handle, buff1, 4, i);
                        if (buff1[0] == float_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 2:

                        memset(buff2, 0, 8);
                        pread64(handle, buff2, 8, i);
                        if (buff2[0] == double_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 3:
                        memset(buff3, 0, 2);
                        pread64(handle, buff3, 2, i);
                        if (buff3[0] == WORD_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 4:
                        memset(buff4, 0, 1);
                        pread64(handle, buff4, 1, i);
                        if (buff4[0] == BYTE_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                }
                if (IsRight_address == 1) {
                    Address_From_FirstSearch_End->Address = i;


                    if (num_count == 0) {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Head = Address_From_FirstSearch_Begin;
                    } else {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin->next = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                    }
                    Address_From_FirstSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

                }


            }
            pointer = (*pointer).next;
        }
        close(handle);
        free(Address_From_FirstSearch_End);
        return Address_From_FirstSearch_Head;
    }
}

MAFS UnionSearch_Filter_value(MAFS Head, int pid, string value, int ValueType) {
    MAFS pointer = Head;
    //mem文件读取
    char lj[128];
    sprintf(lj, "/proc/%d/mem", pid);
    int handle = open(lj, 00000002);

    //返回的新的MAFS
    MAFS Address_From_FilterSearch_End = nullptr;
    MAFS Address_From_FilterSearch_Begin = nullptr;
    MAFS Address_From_FilterSearch_Head = nullptr;
    Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin = Address_From_FilterSearch_End = (MAFS) malloc(
            MemoryAddressFromSearch_Size);


    char value_[] = "";
    String_To_Char(value_, value);


    int IsRight_address = 0;

    int int_value_real;
    float float_value_real;
    double double_value_real;
    WORD WORD_value_real;//需要转换int再转换char
    BYTE BYTE_value_real;//需要转化int再转换short

    switch (ValueType) {
        case 0:
            int_value_real = atoi(value_);
            break;
        case 1:
            float_value_real = atof(value_);
            break;
        case 2:
            double_value_real = hexToDec(value_);
            break;
        case 3:
            WORD_value_real = (WORD) atoi(value_);
            break;
        case 4:
            BYTE_value_real = (BYTE) atoi(value_);
            break;

    }

    if (ValueType == 0) {
        buff[1024] = {0};

    } else if (ValueType == 1) {
        buff1[1024] = {0};

    } else if (ValueType == 2) {
        buff2[1024] = {0};

    } else if (ValueType == 3) {
        buff3[1024] = {0};

    } else if (ValueType == 4) {
        buff4[1024] = {0};

    }


    int num_count = 0;
    while (pointer) {
        long int Address_Filter = (*pointer).Address;

        switch (ValueType) {
            case 0:
                memset(buff, 0, 4);
                pread64(handle, buff, 4, Address_Filter);
                if (buff[0] == int_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 1:
                memset(buff1, 0, 4);
                pread64(handle, buff1, 4, Address_Filter);
                if (buff1[0] == float_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 2:
                memset(buff2, 0, 8);
                pread64(handle, buff2, 8, Address_Filter);
                if (buff2[0] == double_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 3:
                memset(buff3, 0, 2);
                pread64(handle, buff3, 2, Address_Filter);
                if (buff3[0] == WORD_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 4:
                memset(buff4, 0, 1);
                pread64(handle, buff4, 1, Address_Filter);
                if (buff4[0] == BYTE_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
        }

        if (IsRight_address == 1) {
            Address_From_FilterSearch_End->Address = Address_Filter;


            if (num_count == 0) {
                num_count++;
                Address_From_FilterSearch_End->next = nullptr;
                Address_From_FilterSearch_Begin = Address_From_FilterSearch_End;
                Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin;
            } else {
                num_count++;
                Address_From_FilterSearch_End->next = nullptr;
                Address_From_FilterSearch_Begin->next = Address_From_FilterSearch_End;
                Address_From_FilterSearch_Begin = Address_From_FilterSearch_End;
            }
            Address_From_FilterSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

        }

        pointer = (*pointer).next;

    }
    free(Address_From_FilterSearch_End);
    return Address_From_FilterSearch_Head;
};

MAFS UnionSearch_Filter_Rangevalue(MAFS Head, int pid, string value, int ValueType) {

    //返回的新的MAFS
    MAFS Address_From_FilterSearch_End = nullptr;
    MAFS Address_From_FilterSearch_Begin = nullptr;
    MAFS Address_From_FilterSearch_Head = nullptr;
    Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin = Address_From_FilterSearch_End = (MAFS) malloc(
            MemoryAddressFromSearch_Size);


    char value_From[] = "";
    char value_To[] = "";
    char value_[] = "";

    String_To_Char(value_, value);

    string From_Value = mySplit_Result(value_, 0);
    string To_Value = mySplit_Result(value_, 1);

    String_To_Char(value_From, From_Value);
    String_To_Char(value_To, To_Value);

    int int_value_real_From, int_value_real_To;
    float float_value_real_from, float_value_real_to;
    double double_value_real_from, double_value_real_to;
    WORD WORD_value_real_from, WORD_value_real_to;//需要转换int再转换char
    BYTE BYTE_value_real_from, BYTE_value_real_to;//需要转化int再转换short

    switch (ValueType) {
        case 0:
            int_value_real_From = atoi(value_From);
            int_value_real_To = atoi(value_To);

            break;
        case 1:
            float_value_real_from = atof(value_From);
            float_value_real_to = atof(value_To);
            break;
        case 2:
            double_value_real_from = hexToDec(value_From);
            double_value_real_to = hexToDec(value_To);
            break;
        case 3:
            WORD_value_real_from = (WORD) atoi(value_From);
            WORD_value_real_to = (WORD) atoi(value_To);
            break;
        case 4:
            BYTE_value_real_from = (BYTE) atoi(value_From);
            BYTE_value_real_to = (BYTE) atoi(value_To);
            break;

    }

    char lj[128];
    sprintf(lj, "/proc/%d/mem", pid);
    int handle = open(lj, 00000002);

    //此用来计数
    int num_count = 0;

    if (ValueType == 0) {
        buff[1024] = {0};

    } else if (ValueType == 1) {
        buff1[1024] = {0};

    } else if (ValueType == 2) {
        buff2[1024] = {0};

    } else if (ValueType == 3) {
        buff3[1024] = {0};

    } else if (ValueType == 4) {
        buff4[1024] = {0};

    }


    //pread=read+lseek,所以其实可以不用lseek
    //clion中当前编译器的linux内核版本不支持pread64

    MAFS pointer = Head;
    int IsRight_address = 0;

    printf("BeginSearchValue");
    while (pointer) {
        long int address = (*pointer).Address;
        switch (ValueType) {
            case 0:
                memset(buff, 0, 4);
                pread64(handle, buff, 4, address);
                if (buff[0] >= int_value_real_From && buff[0] <= int_value_real_To) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 1:
                memset(buff1, 0, 4);
                pread64(handle, buff1, 4, address);
                if (buff1[0] >= float_value_real_from && buff1[0] <= float_value_real_to) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 2:
                memset(buff2, 0, 8);
                pread64(handle, buff2, 8, address);
                if (buff2[0] >= double_value_real_from && buff2[0] <= double_value_real_to) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 3:
                memset(buff3, 0, 2);
                pread64(handle, buff3, 2, address);
                if (buff3[0] >= WORD_value_real_from && buff3[0] <= WORD_value_real_to) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 4:
                memset(buff4, 0, 1);
                pread64(handle, buff4, 1, address);
                if (buff4[0] >= BYTE_value_real_from && buff4[0] <= BYTE_value_real_to) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
        }

//        //返回的新的MAFS
//        MAFS Address_From_FilterSearch_End = nullptr;
//        MAFS Address_From_FilterSearch_Begin = nullptr;
//        MAFS Address_From_FilterSearch_Head = nullptr;
//        Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin = Address_From_FilterSearch_End = (MAFS)malloc(MemoryAddressFromSearch_Size);



        if (IsRight_address == 1) {
            Address_From_FilterSearch_End->Address = address;
            print("address:");
            cout << (*pointer).Address << endl;
            if (num_count == 0) {
                num_count++;
                Address_From_FilterSearch_End->next = nullptr;
                Address_From_FilterSearch_Begin = Address_From_FilterSearch_End;
                Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin;
            } else {
                num_count++;
                Address_From_FilterSearch_End->next = nullptr;
                Address_From_FilterSearch_Begin->next = Address_From_FilterSearch_End;
                Address_From_FilterSearch_Begin = Address_From_FilterSearch_End;
            }
            Address_From_FilterSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

        }

        pointer = (*pointer).next;

    }
    close(handle);
    free(Address_From_FilterSearch_End);
    return Address_From_FilterSearch_Head;
};

bool EditMemory(MAFS Head, int pid, string value, int ValueType, int WantedEditAddressCout) {
    MAFS pointer = Head;

    char lj[128];
    sprintf(lj, "/proc/%d/mem", pid);
    int handle = open(lj, 00000002);

    char value_[] = "";
    String_To_Char(value_, value);

    int int_value_real;
    float float_value_real;
    double double_value_real;
    WORD WORD_value_real;//需要转换int再转换char
    BYTE BYTE_value_real;//需要转化int再转换short

    switch (ValueType) {
        case 0:
            int_value_real = atoi(value_);
            break;
        case 1:
            float_value_real = atof(value_);
            break;
        case 2:
            double_value_real = hexToDec(value_);
            break;
        case 3:
            WORD_value_real = (WORD) atoi(value_);
            break;
        case 4:
            BYTE_value_real = (BYTE) atoi(value_);
            break;

    }
    int count = 0;
    // try {

    while (pointer) {
        count++;
        if (WantedEditAddressCout != -1) {
            goto BeginWrite;
        } else if (count <= WantedEditAddressCout) {
            goto BeginWrite;
        } else {
            break;
        }

        BeginWrite:
        long int address = (*pointer).Address;
        switch (ValueType) {
            case 0:

                pwrite64(handle, &int_value_real, 4, address);

                break;
            case 1:

                pwrite64(handle, &float_value_real, 4, address);

                break;
            case 2:

                pwrite64(handle, &double_value_real, 8, address);

                break;
            case 3:

                pwrite64(handle, &WORD_value_real, 2, address);

                break;
            case 4:
                pwrite64(handle, &BYTE_value_real, 1, address);

                break;
        }

        pointer = (*pointer).next;
    }


    // } catch (...) {
    //     return false;
    // }
    return true;
}


//String转换成char []
void String_To_Char(char valueChar[], const string &value) {
    strncpy(valueChar, value.c_str(), value.length() + 1);
}


//char转换double
double sqrt(double sum, int i) {
    double root = sum;
    while (i > 0, i--)
        sum *= root;

    return sum;
}

double hexToDec(char *str) {
    int i = 0;
    float sumd = 0.0;
    double sumf = 0.0;
    bool error = false;
    bool negative = false;

    for (; *str; str++) {
        if (*str == '-') {
            negative = true;
            continue;
        }
        if (*str == '.') {
            error = true;
            continue;
        }

        if (error) {
            sumf = sumf + (*str - '0') / sqrt(10.0, i);
            i++;
        } else {
            sumd = 10.0 * sumd + (*str - '0');
        }
    }

    if (negative)
        sumd = -(sumd + sumf);
    else
        sumd += sumf;

    return sumd;
}

//----------------------------------------------------------------------------------------
// 偏移搜索部分
// 先用第一个搜索出来数值，然后存储在链表里面，再搜索偏移的地址即可
MAFS OFFSETSearch_First(int Rangetype, int pid, string value, int ValueType) {
    cout << "Begin Search" << endl;

    MAR Head = SetMemorySearchRange(Rangetype, pid);
    print("GetMARHead-->OK");
    MAFS Address_From_FirstSearch_End = nullptr;
    MAFS Address_From_FirstSearch_Begin = nullptr;
    MAFS Address_From_FirstSearch_Head = nullptr;
    Address_From_FirstSearch_Head = Address_From_FirstSearch_Begin = Address_From_FirstSearch_End = (MAFS) malloc(
            MemoryAddressFromSearch_Size);
    print("PREPARE1-->OK");
    char valueChar[] = "";
    String_To_Char(valueChar, value);
    print("PREPARE2-->OK");
    int num_count = 0;

    if (strstr(valueChar, "~")) {
        char value_From[] = "";
        char value_To[] = "";

        string From_Value = mySplit_Result(valueChar, 0);
        string To_Value = mySplit_Result(valueChar, 1);

        String_To_Char(value_From, From_Value);
        String_To_Char(value_To, To_Value);

        int int_value_real_From, int_value_real_To;
        float float_value_real_from, float_value_real_to;
        double double_value_real_from, double_value_real_to;
        WORD WORD_value_real_from, WORD_value_real_to;//需要转换int再转换char
        BYTE BYTE_value_real_from, BYTE_value_real_to;//需要转化int再转换short

        switch (ValueType) {
            case 0:
                int_value_real_From = atoi(value_From);
                int_value_real_To = atoi(value_To);

                break;
            case 1:
                float_value_real_from = atof(value_From);
                float_value_real_to = atof(value_To);
                break;
            case 2:
                double_value_real_from = hexToDec(value_From);
                double_value_real_to = hexToDec(value_To);
                break;
            case 3:
                WORD_value_real_from = (WORD) atoi(value_From);
                WORD_value_real_to = (WORD) atoi(value_To);
                break;
            case 4:
                BYTE_value_real_from = (BYTE) atoi(value_From);
                BYTE_value_real_to = (BYTE) atoi(value_To);
                break;

        }

        char lj[128];
        sprintf(lj, "/proc/%d/mem", pid);
        int handle = open(lj, 00000002);


        if (ValueType == 0) {
            buff[1024] = {0};

        } else if (ValueType == 1) {
            buff1[1024] = {0};

        } else if (ValueType == 2) {
            buff2[1024] = {0};

        } else if (ValueType == 3) {
            buff3[1024] = {0};

        } else if (ValueType == 4) {
            buff4[1024] = {0};

        }


        //pread=read+lseek,所以其实可以不用lseek
        //clion中当前编译器的linux内核版本不支持pread64

        MAR pointer = Head;
        int IsRight_address = 0;

        printf("BeginSearchValue");
        while (pointer) {
            for (long int i = (*pointer).Begin_address; i <= (*pointer).End_address; i = i + 0x4) {
                switch (ValueType) {
                    case 0:
                        memset(buff, 0, 4);
                        pread64(handle, buff, 4, i);
                        if (buff[0] >= int_value_real_From && buff[0] <= int_value_real_To) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 1:
                        memset(buff1, 0, 4);
                        pread64(handle, buff1, 4, i);
                        if (buff1[0] >= float_value_real_from && buff1[0] <= float_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 2:
                        memset(buff2, 0, 8);
                        pread64(handle, buff2, 8, i);
                        if (buff2[0] >= double_value_real_from &&
                            buff2[0] <= double_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 3:
                        memset(buff3, 0, 2);
                        pread64(handle, buff3, 2, i);
                        if (buff3[0] >= WORD_value_real_from && buff3[0] <= WORD_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 4:
                        memset(buff4, 0, 1);
                        pread64(handle, buff4, 1, i);
                        if (buff4[0] >= BYTE_value_real_from && buff4[0] <= BYTE_value_real_to) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                }
                if (IsRight_address == 1) {
                    Address_From_FirstSearch_End->Address = i;
                    print("address:");
                    cout << i << endl;
                    if (num_count == 0) {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Head = Address_From_FirstSearch_Begin;
                    } else {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin->next = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                    }
                    Address_From_FirstSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

                }

            }
            pointer = (*pointer).next;

        }
        close(handle);
        free(Address_From_FirstSearch_End);
        return Address_From_FirstSearch_Head;


    } else {
        char value_[] = "";


        String_To_Char(value_, value);


        int int_value_real;
        float float_value_real;
        double double_value_real;
        WORD WORD_value_real;//需要转换int再转换char
        BYTE BYTE_value_real;//需要转化int再转换short

        switch (ValueType) {
            case 0:
                int_value_real = atoi(value_);
                break;
            case 1:
                float_value_real = atof(value_);
                break;
            case 2:
                double_value_real = hexToDec(value_);
                break;
            case 3:
                WORD_value_real = (WORD) atoi(value_);
                break;
            case 4:
                BYTE_value_real = (BYTE) atoi(value_);
                break;

        }

        char lj[128];
        sprintf(lj, "/proc/%d/mem", pid);
        int handle = open(lj, 00000002);


        if (ValueType == 0) {
            buff[1024] = {0};

        } else if (ValueType == 1) {
            buff1[1024] = {0};

        } else if (ValueType == 2) {
            buff2[1024] = {0};

        } else if (ValueType == 3) {
            buff3[1024] = {0};

        } else if (ValueType == 4) {
            buff4[1024] = {0};

        }


        //pread=read+lseek,所以其实可以不用lseek
        //clion中当前编译器的linux内核版本不支持pread64

        MAR pointer = Head;
        int IsRight_address = 0;

//        int int_value_real;
//        float float_value_real;
//        double double_value_real;
//        WORD WORD_value_real;//需要转换int再转换char
//        BYTE BYTE_value_real;//需要转化int再转换short
        while (pointer) {
            for (long int i = (*pointer).Begin_address; i <= (*pointer).End_address; i = i + 0x4) {
                switch (ValueType) {
                    case 0:
                        memset(buff, 0, 4);
                        pread64(handle, buff, 4, i);
                        if (buff[0] == int_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 1:
                        memset(buff1, 0, 4);
                        pread64(handle, buff1, 4, i);
                        if (buff1[0] == float_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 2:

                        memset(buff2, 0, 8);
                        pread64(handle, buff2, 8, i);
                        if (buff2[0] == double_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 3:
                        memset(buff3, 0, 2);
                        pread64(handle, buff3, 2, i);
                        if (buff3[0] == WORD_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                    case 4:
                        memset(buff4, 0, 1);
                        pread64(handle, buff4, 1, i);
                        if (buff4[0] == BYTE_value_real) {
                            IsRight_address = 1;
                        } else {
                            IsRight_address = 0;
                        }
                        break;
                }
                if (IsRight_address == 1) {
                    Address_From_FirstSearch_End->Address = i;
                    cout << i << endl;


                    if (num_count == 0) {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Head = Address_From_FirstSearch_Begin;
                    } else {
                        num_count++;
                        Address_From_FirstSearch_End->next = nullptr;
                        Address_From_FirstSearch_Begin->next = Address_From_FirstSearch_End;
                        Address_From_FirstSearch_Begin = Address_From_FirstSearch_End;
                    }
                    Address_From_FirstSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

                }


            }
            cout << "next" << endl;
            pointer = (*pointer).next;
        }
        close(handle);
        free(Address_From_FirstSearch_End);
        return Address_From_FirstSearch_Head;
    }
}

MAFS OFFSETSearch_value(MAFS Head, int pid, string value, int ValueType, int offset) {
    MAFS pointer = Head;
    //mem文件读取
    char lj[128];
    sprintf(lj, "/proc/%d/mem", pid);
    int handle = open(lj, 00000002);

    //返回的新的MAFS
    MAFS Address_From_FilterSearch_End = nullptr;
    MAFS Address_From_FilterSearch_Begin = nullptr;
    MAFS Address_From_FilterSearch_Head = nullptr;
    Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin = Address_From_FilterSearch_End = (MAFS) malloc(
            MemoryAddressFromSearch_Size);


    char value_[] = "";
    String_To_Char(value_, value);


    int IsRight_address = 0;

    int int_value_real;
    float float_value_real;
    double double_value_real;
    WORD WORD_value_real;//需要转换int再转换char
    BYTE BYTE_value_real;//需要转化int再转换short

    switch (ValueType) {
        case 0:
            int_value_real = atoi(value_);
            break;
        case 1:
            float_value_real = atof(value_);
            break;
        case 2:
            double_value_real = hexToDec(value_);
            break;
        case 3:
            WORD_value_real = (WORD) atoi(value_);
            break;
        case 4:
            BYTE_value_real = (BYTE) atoi(value_);
            break;

    }

    if (ValueType == 0) {
        buff[1024] = {0};

    } else if (ValueType == 1) {
        buff1[1024] = {0};

    } else if (ValueType == 2) {
        buff2[1024] = {0};

    } else if (ValueType == 3) {
        buff3[1024] = {0};

    } else if (ValueType == 4) {
        buff4[1024] = {0};

    }


    int num_count = 0;
    while (pointer) {
        long int Address_Filter = (*pointer).Address;

        switch (ValueType) {
            case 0:
                memset(buff, 0, 4);
                pread64(handle, buff, 4, Address_Filter + offset);
                if (buff[0] == int_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 1:
                memset(buff1, 0, 4);
                pread64(handle, buff1, 4, Address_Filter + offset);
                if (buff1[0] == float_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 2:
                memset(buff2, 0, 8);
                pread64(handle, buff2, 8, Address_Filter + offset);
                if (buff2[0] == double_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 3:
                memset(buff3, 0, 2);
                pread64(handle, buff3, 2, Address_Filter + offset);
                if (buff3[0] == WORD_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
            case 4:
                memset(buff4, 0, 1);
                pread64(handle, buff4, 1, Address_Filter + offset);
                if (buff4[0] == BYTE_value_real) {
                    IsRight_address = 1;
                } else {
                    IsRight_address = 0;
                }
                break;
        }

        if (IsRight_address == 1) {
            Address_From_FilterSearch_End->Address = Address_Filter;
            cout << Address_Filter << endl;


            if (num_count == 0) {
                num_count++;
                Address_From_FilterSearch_End->next = nullptr;
                Address_From_FilterSearch_Begin = Address_From_FilterSearch_End;
                Address_From_FilterSearch_Head = Address_From_FilterSearch_Begin;
            } else {
                num_count++;
                Address_From_FilterSearch_End->next = nullptr;
                Address_From_FilterSearch_Begin->next = Address_From_FilterSearch_End;
                Address_From_FilterSearch_Begin = Address_From_FilterSearch_End;
            }
            Address_From_FilterSearch_End = (MAFS) malloc(MemoryAddressFromSearch_Size);

        }

        pointer = (*pointer).next;

    }
    free(Address_From_FilterSearch_End);
    return Address_From_FilterSearch_Head;
};


int main(int argc, char *argv[]) {
    if (argc < 4) {
        cout << "need 3+ arguments!" << endl;
        return -1;
    }
    char *type = argv[1];
    char *value = argv[2];
    char *packageName = argv[3];
    int pid = getPID(packageName);
    if (strcmp(type, "s") == 0) {
        MAFS addressList = UnionSearch_First(0, pid, value, 0);
        Print_Linked_list_MAFS(addressList);
    } else if (strcmp(type, "f") == 0) {
        MAFS addressHeader = (MAFS) malloc(MemoryAddressFromSearch_Size);
        addressHeader->Address = atoi(argv[4]);
        addressHeader->next = nullptr;
        MAFS addressNow = addressHeader;
        for (int i = 5; i < argc; i++) {
            MAFS address = (MAFS) malloc(MemoryAddressFromSearch_Size);
            address->Address = atoi(argv[i]);
            address->next = nullptr;
            addressNow->next = address;
            addressNow = address;
        }

        MAFS addressList = UnionSearch_Filter_value(addressHeader, pid, value, 0);
        Print_Linked_list_MAFS(addressList);
    } else if (strcmp(type, "e") == 0) {
        MAFS addressHeader = (MAFS) malloc(MemoryAddressFromSearch_Size);
        addressHeader->Address = atoi(argv[4]);
        addressHeader->next = nullptr;
        MAFS addressNow = addressHeader;
        for (int i = 5; i < argc; i++) {
            MAFS address = (MAFS) malloc(MemoryAddressFromSearch_Size);
            address->Address = atoi(argv[i]);
            address->next = nullptr;
            addressNow->next = address;
            addressNow = address;
        }

        bool isOk = EditMemory(addressHeader, pid, value, 0, 10);
        if (isOk) {
            print("Edit --> ok");
        } else {
            print("Edit --> false");
        }
    } else {
        cout << "unknow type!" << endl;
    }


    /*
    int Mode = 4;
    if (Mode == 0) {
        StopPID(pid);


        //修改的方法：以GG修改器为例
//      gg.setrange(BAD)
//    1.gg.search("1.1F;1.2~1.3F;3D")
//    2.gg.search("1.2~1.3F")
//    3.gg.getResualt(30)
//    4.gg.edit("3D")


        MAFS Address_From_Fs = UnionSearch_First(1, pid, "1.1", 1);
        MAFS Address_From_Fs1 = UnionSearch_First(1, pid, "1.2~1.3", 1);
        MAFS Address_From_Fs2 = UnionSearch_First(1, pid, "3", 0);

        MAFS ADD;
        MAFS ADD1;
        MergeLinkList_LL(Address_From_Fs, Address_From_Fs1, ADD);
        MergeLinkList_LL(ADD, Address_From_Fs2, ADD1);

        string ab = "1.2~1.3";


        MAFS ADD2;
        ADD2 = UnionSearch_Filter_Rangevalue(ADD1, pid, ab, 1);



        //Print_Linked_list_MAFS(Address_From_Fs);


        bool Is_ok = EditMemory(Address_From_Fs, pid, "2", 0, 30);
        if (Is_ok) {
            print("Edit ok --> ok");
        } else {
            print("Edit --> false");
        }
        ResetFromStopPID(pid);
    } else if (Mode == 1) {
        MAFS Address_From_Fs = UnionSearch_First(0, pid, "1235800", 0);
        Print_Linked_list_MAFS(Address_From_Fs);
        bool Is_ok = EditMemory(Address_From_Fs, pid, "2", 0, 30);
        if (Is_ok) {
            print("Edit ok --> ok");
        } else {
           print("Edit --> false");
        }
    } else if (Mode == 2) {
        print("begin offset search");
        MAFS Address_From_Fs = OFFSETSearch_First(0, pid, "1235800~12358009", 0);
        MAFS Address_From_Fs1 = OFFSETSearch_value(Address_From_Fs, pid, "1235800~12358009", 0, -8);
        MAFS Address_From_Fs2 = OFFSETSearch_value(Address_From_Fs1, pid, "1235800~12358009", 0, -12);
        MAFS Address_From_Fs3 = OFFSETSearch_value(Address_From_Fs2, pid, "1235800~12358009", 0, -16);
        bool Is_ok = EditMemory(Address_From_Fs3, pid, "999999999", 0, -1);
        if (Is_ok) {
            print("Edit ok --> ok");
        } else {
            print("Edit --> false");
        }
        Address_From_Fs = nullptr;
        Address_From_Fs1 = nullptr;
        Address_From_Fs2 = nullptr;
        Address_From_Fs3 = nullptr;

    }
    */

    return 0;
}


