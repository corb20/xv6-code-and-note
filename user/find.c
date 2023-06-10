#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *);

void find(char* path,char*filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    char bufname[512];

    if((fd = open(path, 0)) < 0){
      fprintf(2, "find: cannot open %s\n", path);
      return;
    }

    if(fstat(fd, &st) < 0){
      fprintf(2, "find: cannot stat %s\n", path);
      close(fd);
      return;
    }
    switch(st.type){
        case T_DIR://如果这个path是一个文件夹
            //列出该文件夹下的所有内容，对所有的文件名不是filename的执行新一轮的find
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                strcpy(bufname,fmtname(buf));
                printf("the file in dir %s\n", bufname);
                //printf("buf: %s\n", buf);
                printf("bufname cmp: %d\n",strcmp(bufname, "."));
                if (strcmp(bufname, ".") == 0 || strcmp(bufname, "..") == 0)
                    continue;
                //printf("is it .?");
                if (strcmp(bufname, filename) == 0)
                {
                    printf("%s/%s\n",path,filename);
                }
                else{
                    find(buf, filename);
                }
            }
            break;
        default:
            break;
    }
    close(fd);
    return;
}

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = '\0';
  return buf;
}

int main(int argc, char** argv)
{
    if(argc<3){
        printf("argc error 'find' needs 2 parameters");
        exit(-1);
    }

    //argv[1]是需要去查询的目录
    //argv[2]是需要找到的文件名

    //可以用递归去查找
    if(strcmp(argv[1], ".")==0)
        find(".", argv[2]);
    exit(0);
}