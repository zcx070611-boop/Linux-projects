#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>
#include<unistd.h>
#include<sys/types.h>
#include <sys/wait.h>
#include<ctype.h>
#include <fcntl.h>
#define MAX_ARGC 128
#define MAX_P 100
#define NoneRedir 0
#define InputRedir 1
#define OutputRedir 2
#define AppRedir 3
int redir=NoneRedir;
char*filename=nullptr;
using namespace std;
char hostname[256];
char command[1024];
char* argv[MAX_ARGC];//命令行参数
int argc=0;//命令行参数个数
int lastcode=0;
char CWD[MAX_P];
char PWD[MAX_P];
char OLDPWD[MAX_P];
string GetName()
{
    string name=getenv("USER");
    return name.empty()?"None":name;
}
string GetPwd()
{
    if (getcwd(PWD, sizeof(PWD)) == NULL)
    {
        perror("getcwd");
        return "";
    }
    return PWD;
}

string GetHome()
{
    string home=getenv("HOME");
    return home.empty()?"None":home;
}
string GetHost()
{
    if(gethostname(hostname,sizeof(hostname))==0)
    {
        return hostname;
    }
    return "None";
}
//打印命令行
void PrintCommandPrompt()
{
    char commandprompt[128];
    memset(commandprompt,0,sizeof(commandprompt));
    snprintf(commandprompt,sizeof(commandprompt),"%s@%s:%s$",GetName().c_str(),GetHost().c_str(),GetPwd().c_str());
    printf("%s",commandprompt);
}
//获取命令
bool GetCommand(char*buffer,int size)
{
    char*s=fgets(buffer,size,stdin);
    if(s==NULL)
    {
        return false;
    }
    s[strcspn(buffer,"\n")]='\0';
    if(strlen(buffer)==0)
    {
        return false;
    }
    return true;
}
//命令分割
bool CommandParse(char*command)
{
    argc=0;
    argv[argc++]=strtok(command," ");
    while(bool(argv[argc++]=strtok(nullptr," ")));
    argc--;
    return argc>0?true:false;
}
void DoRedir();
//执行命令
int Execute()
{
    pid_t ret=fork();
    if(ret==0)
    {
         DoRedir();
        execvp(argv[0],argv);
        exit(1);//创建子进程失败时返回1，成功时不会执行到这一步。
    }
    //父进程，实现lastcode（上一条进程的返回码）
    int status=0;
    pid_t id=waitpid(ret,&status,0);
    if(id>0)
    {
        lastcode = WEXITSTATUS(status);
    }
    return 0;
}
bool Cd()
{
    memset(PWD,0,MAX_P);
    if(argc==1)
    {
       string home =GetHome();
       if(home=="None") return true;
       snprintf(OLDPWD,sizeof(OLDPWD),"OLDPWD=%s",getcwd(CWD,sizeof(CWD)));
       putenv(OLDPWD);
       chdir(home.c_str());
       snprintf(PWD,sizeof(PWD),"PWD=%s",home.c_str());
       putenv(PWD);
    }
    else
    {
        char* where=argv[1];
        if(where=="~")
        {
           string home = GetHome();
           if(home.empty()) return true;
           snprintf(OLDPWD, sizeof(OLDPWD), "OLDPWD=%s", getcwd(CWD, sizeof(CWD))); // 在更新路径之前记录OLDPWD
           putenv(OLDPWD);
           chdir(home.c_str());
           snprintf(PWD, sizeof(PWD), "PWD=%s", home.c_str());
           putenv(PWD);
        }
        else if(where=="-")
        {
            string oldpwd=getenv("OLDPWD");
            if(oldpwd.empty()) return true;
            snprintf(OLDPWD,sizeof(OLDPWD),"OLDPWD=%s",getcwd(CWD,sizeof(CWD)));
            putenv(OLDPWD);
            chdir(oldpwd.c_str());
            snprintf(PWD,sizeof(PWD),"PWD=%s",oldpwd.c_str());
            putenv(PWD);
        }
        else
        {
            snprintf(OLDPWD,sizeof(OLDPWD),"OLDPWD=%s",getcwd(CWD,sizeof(CWD)));
            putenv(OLDPWD);
            chdir(where);
            snprintf(PWD,sizeof(PWD),"PWD=%s",where);
            putenv(PWD);
        }
     }
    return true;
}
bool Echo()
{
    if(argc == 1) return false;
    string s = argv[1];
    if(s == "$?")
    printf("%d\n", lastcode);
    else if(s[0] == '$')
    {
       string ss = "";
       for(int i = 1; i < s.size(); i++)
           ss += s[i];
       if(getenv(ss.c_str()))
           printf("%s\n",getenv(ss.c_str()));
       else return false;
    }
    else
    {
        string ss = "";
        int i = 1;
        while(argv[i])
        {
            string tmp = argv[i];
            ss += tmp + " ";
            i++;
        }
        printf("%s\n", ss.c_str());
    }
        return true;
}
//判断内建
bool CheckAndExecBuiltin()
{
    string cmd=argv[0];
    if(cmd=="cd")
    {
        Cd();
        return true;
    }
    else if(cmd=="echo")
    {
        Echo();
        return true;
    }
    else
    {
        return false;
    }
}
//找到第一个非空格的地方（即 文件名开始）
char*Trimspace(char*pos)
{
    while(isspace(*pos))
    {
        pos++;
    }
    return pos;
}
// 分割命令行，并且分析重定向类型  
void PraseRedir(char command_buffer[],int len)
{
    int end=len-1;
    while(end>=0)
    {
        if(command_buffer[end]=='<')
        {
            redir=InputRedir;
            command_buffer[end]=0;
            filename=Trimspace(&command_buffer[end]+1);
            break;
        }
        else if(command_buffer[end]=='>')
        {
            if(command_buffer[end-1]=='>')
            {
                redir=AppRedir;
                command_buffer[end-1]=0;
                filename=Trimspace(&command_buffer[end]+1);
                break;
            }
            else
            {
            redir=OutputRedir;
            command_buffer[end]=0;
            filename=Trimspace(&command_buffer[end]+1);
            break;
            }
        }
        else
        {
            end--;
        }
    }
}  
void DoRedir()
{
    if(redir==InputRedir)
    {
        if(filename)
        {
            int fd=open(filename,O_RDONLY);
            if(fd<0)
            {
                exit(2);
            }
            dup2(fd,0);
        }
        else
        {
            exit(1);
        }
    }
    else if(redir==OutputRedir)
    {
        if(filename)
        {
            int fd=open(filename,O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if(fd<0)
            {
                exit(4);
            }
            dup2(fd,1);
        }
        else
        {
            exit(3);
        }  
    }
    else if(redir==AppRedir)
    {
        if(filename)
        {
            int fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0666);
            if(fd < 0) exit(6);
            dup2(fd, 1);
        }
        else exit(5);
    }
    else
    {
    }
}
int main()
{
    while (true)
    {
        PrintCommandPrompt();
        if (!GetCommand(command, sizeof(command)))
            break;                    // Ctrl+D 时 GetCommand 返回 false，跳出循环退出
        redir = NoneRedir;                       // ① 每轮先清重定向状态
        filename = nullptr;
        PraseRedir(command, strlen(command));
        CommandParse(command);
        if (argc == 0) continue;      // 空行（直接回车）不处理，重新来一轮
        if (!CheckAndExecBuiltin())   // 不是内建命令
            Execute();                // 才走 fork + execvp 跑外部命令
    }
    return 0;
}

