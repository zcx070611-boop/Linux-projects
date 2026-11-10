#include "my_stdio.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
    mFILE *fp = mfopen("./log.txt", "a");
    if(fp == NULL)
    {
        return 1;
    }
    int cnt = 10;
    while(cnt)
    {
        printf("write %d\n", cnt);
        char buffer[64];
        snprintf(buffer, sizeof(buffer),"hello message, number is : %d", cnt);
        cnt--;
        mfwrite(buffer, strlen(buffer), fp);
        mfflush(fp);
        sleep(1);
    }
    mfclose(fp);
}
