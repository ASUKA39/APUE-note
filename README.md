# Linux 系统编程笔记

- 课程视频：[Linux 系统编程 - 李慧琴老师](https://www.bilibili.com/video/BV1yJ411S7r6)
  - 深入浅出，点面结合，恪守标准，爆赞
- 参考书目
  - UNIX 环境高级编程（第三版）
  - Linux 内核设计与实现（第三版）
  - 深入理解 Linux 内核（第三版）
- 参考文章
  - [Linux系统编程学习笔记 | 来生拓己 オフィシャルサイト](https://kisugitakumi.net/2022/11/20/Linux系统编程学习笔记/)

## 标准 I/O

### 简介

- I/O：input & output
- stdio：标准 I/O（优先使用，因为可移植性好且封装性好）
- sysio：系统 I/O（也叫文件 I/O）

常见标准 I/O：

| 打开/关闭文件 | 输入输出流          | 文件指针操作 | 缓存相关 |
| ------------- | ------------------- | ------------ | -------- |
| fopen         | fgetc，fputc        | fseek        | fflush   |
| fclose        | fgets，fputs        | ftell        |          |
|               | fread，fwrite       | rewind       |          |
|               | printf 族，scanf 族 |              |          |

FILE 类型始终贯穿标准 I/O

### fopen

```c
FILE *fopen(const char *path, const char *mode)
```

- path：文件路径
- mode：访问权限
  - r：只读，文件指针定位到文件开头，要求文件必须存在
  - r+：可读写，文件指针定位到文件开头，要求文件必须存在
  - w：只写，有此文件则清空，无此文件则创建文件，文件指针定位到文件开头
  - w+：可读写，有此文件则清空，无此文件则创建文件，文件指针定位到文件开头
  - a：只写，追加到文件，无此文件则创建文件，文件指针定位到文件末尾（最后一个字节的下一个位置）
  - a+：可读可追加（可写），无此文件则创建文件，读文件加时文件指针定位到文件开头，追加时文件指针定位到文件末尾
  - b：以二进制流打开，可以在以上权限后面加此权限（遵循 POSIX 的系统可以忽略，包括 Linux）
- 若执行成功函数返回一个 FILE 指针，失败则返回 NULL 并设置全局变量`errno`
  - `errno`的定义在 `/usr/include/asm-generic`的宏中，若想使用需包含`errno.h`
  - `perror()`可以输出易读的`errno`信息，包含在`stdio.h`中
  - `strerror()`可以返回一个易读的`errno`信息的字符串，包含在`string.h`中
- 优秀的编码规范：const 指针表明函数不会改变路径或访问权限

新建出的文件的权限（RWX，通过`ls -l`查看的那个）

- 公式：`0666 & ~umask`（若创建的是目录则为`0777 & ~umask`）
- 公式中的数字全是八进制数
- umask（权限掩码）是用户创建的文件的默认权限，可以使用`umask`指令查看
- 可见，umask 越大，创建的文件的权限越少
- umask 在`/etc/profile`中有定义：UID 大于 199 的用户默认为 002（文件 664、目录 775），其他用户默认为 022（文件 644、目录 755）

### fclose

```c
int fclose(FILE *fp);
```

* 若执行成功函数返回 0，失败则返回 EOF（一般是 -1，也会有例外）并设置`errno`
* 如果文件是输出流，则会表现为先刷新缓冲区再关闭文件

为什么要有`fclose()`？因为实际上`fopen()`内隐含了一个`malloc()`，故打开的文件内存位于堆中；同理，`fclose()`需隐含一个`free()`来释放内存

Linux 会对文件描述符的数量进行限制，可以通过`ulimit -n`命令查看，超过限制数则会弹出`too many open files`警告，每个用户的文件数目上限可以在`/etc/security/limits.conf`中修改
在默认情况下至多可以打开 3 + 1021 = 1024 个文件，上限可修改的极限为 1024 * 1024 = 1048576

### fgetc、fputc

```c
int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar(void);
```

这三个函数若执行成功则返回读到的 unsigned char 强转 int，失败或读到文件尾则返回 EOF

`getchar()`等价于`getc(stdin)`
`getc()`等价于`fgetc()`，但`getc()`是由宏来实现的，而`fgetc()`是正常的函数实现，相对而言宏实现编译更慢运行更快

```c
int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
```

`putchar(c)`等价于`putc(c, stdout)`
同上，`putc()`等价于`fputc()`，区别是宏实现与函数实现

#### 例程：实现 cp 指令

```c
#include <stdio.h>
#include <stdlib.h>
 
int main(int argc, char **argv){
    FILE *fps, *fpd;
    int ch;

    if(argc < 3){
        fprintf(stderr, "Usage:%s <src_file> <dest_file>\n", argv[0]);
        exit(1);
    }
    
    fps = fopen(argv[1], "r");
    if(fps == NULL){
        perror("fopen()");
        exit(1);
    }
    fpd = fopen(argv[2], "w");
    if(fps == NULL){
        fclose(fps);
        perror("fopen()");
        exit(1);
    }

    while(1){
        ch = fgetc(fps);
        if(ch == EOF){
            break;
        }
        fputc(ch, fpd);
    }
    
    fclose(fpd);
    fclose(fps);
    return 0;
}
```

![mycp](./img/p1/mycp.png)

### fgets、fputs

```c
char *fgets(char *s, int size, FILE *stream);
char *gets(char *s);
```

`gets`是危险的，建议使用`fgets()`

`fgets()`接收 stdin 时会在读到`\n`或读到 size - 1 时补上`\0`并停止
函数成功时返回串的指针，失败或读到文件末尾或未接收到任何字符则返回空指针
易错点：假设不停调用`fgets()`，输入 size - 1 个字符加换行会先在读到 size - 1 时结束然后在读到`\n`时再结束一次）

```c
int fputs(const char *s, FILE *stream);
int puts(const char *s);
```

### fread、fwrite

二进制流/文件流的输入输出

```c
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
```

- `fread()`：ptr 从 stream 读 nmemb 个对象，每个对象的大小为 size 字节
- `fwrite()`：ptr 向 stream 写 nmemb 个对象，每个对象的大小为 size 字节
- 读写数量以对象为单位，若文件剩余字节数不足以读写一个对象则会直接结束
- 函数返回已读写的对象数，执行失败或读到文件尾则返回 0

#### 修改例程 mycp

用`fread()`和`fwrite()`代替`fgetc`和`fputc`

```c
char buf[SIZE];
···
while((n = fread(buf, 1, SIZE, fps)) > 0)
	fwrite(buf, 1, n, fpd);
```

### printf 族、scanf 族

```c
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *fromat, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
```

- `printf`：将格式化串输出到 stdout
- `fprintf`：将格式化串输出到 stream 中，可以用于实现重定向
- `sprintf`：将格式化串输出到串中
  - 示例：`sprintf(str, "%d + %d = %d", 1, 2, 3);`->`str == "1 + 2 = 3"`
  - `atoi`：字符串转整数，如："123456" -> 123456、"123a456" -> 123
- `snprintf`：带对象数限制的`sprintf`

```c
int scanf(const char *format, ...);
int fscanf(FILE *stream, const char *fromat, ...);
int sscanf(char *str, const char *format, ...);
```

- `scanf`、`fscanf`：同上
- `sscanf`：按照格式化串将串内容输入变量中
  - 示例：
    `strcpy(dtm, "Saturday March 25 1989");`
    `sscanf(dtm, "%s %s %d %d", weekday, month, &day, &year);`
  - 结果：`printf("%s %d, %d = %s\n", month, day, year, weekday);`->`March 25, 1989 = Saturday`

### fseek、ftell

```c
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
```

- `fseek`：将文件指针位置设置到相对`whence`偏移`offset`处，执行成功返回 0
  - `whence`：文件开头`SEEK_SET`、文件指针当前位置`SEEK_CUR`、文件末尾`SEEK_END`（正负偏移是允许的）
  - 文件读写会导致文件指针的移动，此时常使用`fseek`重新定位文件指针
- `ftell`：返回文件指针当前位置
- `rewind`：等价于`(void)fseek(stream, 0L, SEEK_SET)`，即将文件指针设置到文件开始处

```c
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftell(FILE *stream);
```

- 由于历史原因，此前设计选用的 long 类型只能支持 2G 以内的文件，无法满足现代系统的文件大小，故用 off_t 类型支持更大的文件（off_t 大小各系统不同，64 bit Linux 中是 64 位），编译时加上`_FILE_OFFSET_BITS`宏可以修改 off_t 大小，
- 仅 POSIX 支持，C89、C99 不支持（即所谓方言）

### fflush

```c
int fflush(FILE *stream);
```

- 将缓冲区刷新到文件流中，若 stream 为 NULL 则会刷新打开的所有文件

stdout 是行缓冲模式，只在遇到`\n`、`\0`或行满时才会刷新输出流

示例

```c
printf("before");
while(1);
printf("after");
```

以上情况无输出，因为 stdout 未被刷新

```c
printf("before\n");
while(1);
printf("after\n");
```

或

```c
printf("before");
fflush(stdout);
// or fflush(NULL);
while(1);
printf("after");
```

则可以正常输出 "before"，因为两种方式都刷新了 stdout

关于缓冲区：

- 缓冲区是在内存中一块指定大小的存储空间，用于临时存储 I/O 数据
- 作用：合并系统调用、实现数据块复用、缓和 CPU 与 I/O 设备的速度差异
- 模式：
  - 行缓冲：换行、满时或强制刷新时刷新（如 stdout）
  - 全缓冲：满时刷新或强制刷新（默认，只要不是终端设备）
  - 无缓冲：立即输出（如 stderr）
- 强制刷新时换行符仅是一个字符，无刷新功能
- 进程结束时也会强制刷新
- 更改缓冲区：（了解即可）
  - `int setvbuf(FILE *stream, char *buf, int mode, size_t size);`
  - buf：分配给用户的缓冲，NULL 时自动分配默认大小
  - mode：无缓冲`_IONBF`、行缓冲`_IOLBF`、全缓冲`_IOFBF`

### getline

```c
#define _GNU_SOUECE
#include <stdio.h>

ssize_t getline(char **lineptr, size_t *n, FILE *stream);
```

- lineptr：指向存放字符串行的指针，若为 NULL 则由操作系统 malloc（由于是动态分配内存，所以串的指针值很可能会因 realloc 而产生变化，故这里采用二级指针），使用完函数记得 free 掉曾使用的内存
- n：指定缓冲区的大小，若为系统自行 malloc 则填 0
- `getline`会生成从输入流读入的一整行字符串，在文件结束、遇到定界符或达到最大限度时结束生成字符串，成功执行则返回读到的字符数（包括回车符等定界符，但不包括`\0`），失败则返回 -1
- `getline`依旧不被标准 C 支持，但 POSIX 和 C++ 支持
- `getline`可以动态地调整缓冲区大小，这是此前函数不支持的
- `_GNU_SOUECE`宏建议在 Makefile 中添加，但其实现代编译器已经默认支持该宏

示例

```c
int main(int argc, char **argv) {
    FILE *fp;

    char *linebuf = NULL;
    size_t linesize = 0;
    
    if(argc < 2) {
        fprintf(stderr, "Usage...\n");
    }
    
    fp = fopen(argv[1], "r");
    if(fp == NULL) {
        perror("fopen()");
        exit(1);
    }
    
    while(1) {
    	if(getline(&linebuf, &linesize, fp) < 0)
            break;
       	printf("%d\n", strlen(linebuf));
    }
    
    fclose(fp);
    exit(0);
}
```

局部变量使用前一定要初始化
进程结束会自动释放内存，故这里没有手动 free 掉

### 临时文件

```c
char *tmpnam(char *s);
FILE *tmpfile(void);
```

- `tmpnam`会生成并返回一个有效的临时文件名
  - s 为存放文件名的字符串，创建文件名成功会将其返回，若 s 为 NULL 则指向缓冲区，其会在下一次调用函数时被覆盖
  - 函数存在并发问题，多线程下不做处理有可能产生两个甚至更多同名临时文件，造成覆盖
- `tmpfile`会产生一个匿名文件
  - 以 w+b 模式创建临时文件，失败将返回 NULL
  - 匿名文件不可见，但在磁盘中，同样有文件描述符和打开计数器等
  - 在被关闭或进程结束时匿名文件会被自动删除

## 系统调用 I/O

### 文件描述符

- 设计`open`函数打开一个文件的过程中必然会产生一个结构体（文件表项）来描述这个文件，这个结构体记录了包括 inode、引用计数在内的文件操作信息（类似于 FILE 结构体），这些结构体的地址会被存放在进程内部的一个数组中，而文件描述符 fd 实际上就是这个数组的索引
- 文件描述符是整型的，但`ulimit`会限制一个用户可打开的文件数量，默认 1024
- `fopen`内部实际上也调用了`open`，区别在于`fopen`只能打开流式文件，而`open`能打开设备文件等
- 每个进程都有自己的描述符表（但会共享一个操作系统维护的文件表），负责记录进程打开的所有文件
- 文件描述符会优先选择最小的空位，一个文件可以有多个描述符
- 调用`close`时会将引用计数减 1，如果此时引用计数为 0 则释放文件

### open、close

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
```

- flags 和 mode 都是以位图形式表示的，就是 kernel 代码里那种或一大堆宏名然后用掩码提取的形式
- `open`是变参函数。变参和 C++ 重载的区别在于变参函数在编译时不会限制传入参数的数量（不报错不代表是正确写法）但重载会严格限制所定义的参数个数；最典型的变参函数就是`printf`
- flags：包含必需访问模式和可选模式，用按位或运算串联
  - 必需：只读`O_RDONLY`、只写`WRONLY`、读写`O_RDWR`
  - 可选：可分为创建选项和状态选项两大类，数量庞大故不详细列出，常用如下
    - 创建`O_CREAT`、确保创建不冲突`O_EXCL`、追加`O_APPEND`、文件长度置零`O_TRUNC`、非阻塞模式打开`O_NONBLOCK`（默认以阻塞模式打开文件）
  - 对比`fopen`：
    - `r == O_RDONLY`
    - `r+ == O_RDWR`
    - `w == O_WRONLY | O_CREAT | O_TRUNC`
    - `w+ == O_RDWR | O_CREAT | O_TRUNC`
- mode：搭配`O_CREAT`使用，同样是用按位或运算串联，主要规定各种权限

```c
#include <unistd.h>

int close(int fd);
```

- 返回 0 表示执行成功，-1 表示失败（一般认为不会执行失败）

### read、write、lseek

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
```

- 从文件 fd 中取出前 count 字节放入缓冲区 buf 中
- 执行成功返回实际读取的字节数，错误返回 -1

```c
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count);
```

- 从缓冲区 buf 读取前 count 字节写入文件 fd 中
- 执行成功返回实际读取的字节数，错误返回 -1

```c
#include <sys/types.h>
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

- 设置文件位置指针为从 whence 开始的偏移 offset
- whence 可取：
  - `SEEK_SET`：开头
  - `SEEK_END`：末尾
  - `SEEK_CUR`：当前位置

#### 例程：使用 read、write 实现 mycp

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 1024

int main(int argc, char **argv)
{
    int sfd, dfd;
    char buf[BUFSIZE];
    int len, ret, pos;

    if (argc < 3)
    {
        fprintf(stderr, "Usage:%s <src_file> <dest_file>\n", argv[0]);
        exit(1);
    }

    if ((sfd = open(argv[1], O_RDONLY)) < 0)
    {
        perror("open()");
        exit(1);
    }
    if ((dfd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
    {
        close(sfd);
        perror("open()");
        exit(1);
    }

    while (1)
    {
        if ((len = read(sfd, buf, BUFSIZE)) < 0)
        {
            perror("read()");
            break;
        }
        if (len == 0)
            break;

        pos = 0;
        while (len > 0)
        {
            if ((ret = write(dfd, buf + pos, len)) < 0)
            {
                perror("write()");
                exit(1);
            }
            pos += ret;
            len -= ret;
        }
    }

    close(dfd);
    close(sfd);

    exit(0);
}
```

- 毕竟是系统编程，现在开始模仿 kernel 的编码风格
- 在`write`处引入`pos`的原因是防止`write`提前返回导致实际写入的数量少于`len`（比如磁盘空间不足、阻塞的系统调用被信号打断）
- 而不在`read`处不引入`pos`的原因则是文件大小可能小于`BUFSIZE`

### 系统 I/O 与标准 I/O、I/O 效率

系统 I/O（文件 I/O）与标准 I/O 的区别

- 系统 I/O 没有缓存，操作对象是文件描述符`fd`指向的文件结构体，对文件的读写是面向磁盘的，响应速度快但吞吐量小、系统调用频繁
- 标准 I/O 设有缓存，操作对象是封装了文件描述符`fd`以及其他内容的`FILE`结构体，操作往往基于缓存，响应速度慢但吞吐量大
- 相对而言大吞吐量会使用户的使用体验更好，但不是越大越好
  - 可以用 time 命令查看程序执行的总时间（real）、用户态时间（user）和内核态时间（sys）

建议多使用标准 I/O 而少使用文件 I/O
由于 cache 的原因，两者混用会导致同步问题，如果不得不混用可以用以下函数互转

```c
int fileno(FILE *stream);
FILE *fdopen(int fd, const char *mode);
```

示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    putchar('a');
    write(1, "b", 1);

    putchar('a');
    write(1, "b", 1);

    putchar('a');
    write(1, "b", 1);

    exit(0);
}
```

- 预期输出是`ababab`，但实际输出是`bbbaaa`
- 通过`strace`可以跟踪系统调用
  - 三个`write(1, "b", 1)`被立即执行
  - 三个`putchar('a')`是在最后才通过一个`write(1, "aaa", 3)`一次性输出

### truncate、ftruncate

```c
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
```

- 两个函数都用于截短文件长度
  - `trunacte`：截短路径为 path 发文件
  - `ftruncate`：截短已通过文件 I/O 打开的文件

### dup、dup2、原子操作

```c
#include <unistd.h>

int dup(int oldfd);
int dup2(int oldfd, int newfd);
```

- `dup`将文件`oldfd`复制到当前最小的空闲文件描述符并返回之
- `dup2`关闭`newfd`（如果已打开）并将文件`oldfd`复制到`newfd`并返回`newfd`，如果`newfd`与`oldfd`相等则不执行操作并返回`newfd`，与`dup`不同的是`dup2`是原子的
- 以上两个函数并不会关闭`oldfd`，需要手动关闭，若调用`dup2`则关闭时需要注意判断`newfd`与`oldfd`是否相等
- 像这样的操作需要考虑更多情况：
  - 操作的原子性
  - 关闭文件在本线程完成需求后其他线程是否还会使用（线程之间文件描述符是共享的）

### fcntl、ioctl、/dev/fd

```c
#include <unistd.h>
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */);
```

- 以前种种文件操作实际上都是使用`fcntl`通过不同的`cmd`实现的
  - 复制一个已有的描述符：`F_DUPFD`或`F_DUPFD_CLOEXEC`
  - 获取/设置文件描述符标志：`F_GETFD`或`F_SETFD`
  - 获取/设置文件状态标志：`F_GETFL`或`F_SETFL`
  - 获取/设置异步I/O所有权：`F_GETOWN`或`F_SETOWN`
  - 获取/设置记录锁：`F_GETLK`、`F_SETLK`或`F_SETLKW`
- 成功执行返回值依赖于`cmd`，失败则返回 -1

```c
#include <sys/ioctl.h>

int ioctl(int d, int request, ...)
```

- 种种设备操作实际上都是使用`ioctl`实现的
- 由于 Linux 需要对各种设备进行支持，`ioctl`极其杂乱庞大

对于每个进程，内核都会提供一个虚目录`/dev/fd`，进程打开的文件的文件描述符对应其中的文件`/dev/fd/n`中的编号 n
所以以下函数功能是等价的

```c
fd = open("/dev/fd/1", O_WRONLY);
// is equal to
fd = dup(1)
```

## 文件系统

### 简介

#### Linux 目录树

![dir tree](./img/dir tree.png)

- `/bin`：bin 是 Binaries 的缩写，这个目录存放着最经常使用的命令对应的二进制可执行文件
- `/boot`：这里存放的是启动 Linux 时使用的一些核心文件，包括一些连接文件以及 kernel 镜像文件
- `/dev` ：dev 是 Device(设备) 的缩写，该目录下存放的是 Linux 的外部设备，在 Linux 中访问设备的方式和访问文件的方式是相同的（一切皆文件）
- `/etc`：etc 是 Etcetera 的缩写，这个目录用来存放所有的系统管理所需要的配置文件和子目录
  - `/etc/passwd`：记载了用户信息，格式为`[user name]:"x":[uid]:[gid]:[description]:[home dir]:[default shell]`
  - `/etc/shadow`：基本与`/etc/passwd`一致，区别在于 "x" 换成了密码的哈希值与加密时加的盐
  - `/etc/group`：记载了用户组信息，格式为`[group name]:[group password]:[gid]:[group user]`
- `/home`：用户的主目录，每个用户都有一个自己的目录，一般该目录名是以用户的账号命名
- `/lib`：lib 是 Library 的缩写这个目录里存放着系统最基本的动态连接共享库
- `/lost+found`：这个目录一般情况下是空的，当系统非法关机后，这里就存放了一些文件
- `/media`：Linux 系统会自动识别一些设备，例如U盘、光驱等等，当识别后，Linux 会把识别的设备挂载到这个目录下
- `/mnt`：系统提供该目录是为了让用户临时挂载别的文件系统的，我们可以将光驱挂载在 /mnt/ 上，然后进入该目录就可以查`看光驱里的内容了`
- `/opt`：opt 是 Optional 的缩写，这是给主机额外安装的用户级第三方软件所摆放的目录，默认是空的
- `/proc`：proc 是 Processes 的缩写，这是一种虚拟文件系统，存储的是当前内核运行状态的一系列特殊文件，这个目录是一个虚拟的目录，它是系统内存的映射，我们可以通过直接访问这个目录来获取系统信息
- `/root`：root 用户的主目录
- `/sbin`：s 就是 Super User 的意思，是 Superuser Binaries 的缩写，这里存放的是系统管理员使用的系统管理程序
- `/tmp`：tmp 是 Temporary 的缩写这个目录是用来存放一些临时文件的
- `/usr`：usr 是 Unix Shared Resources 的缩写（不是 User，这个经常搞错），这是一个非常重要的目录，用户的很多应用程序和文件都放在这个目录下，类似于 windows 下的 C:/Windows/ 目录
  - `/usr/lib`：可以理解为 C:/Windows/System32
  - `/usr/local`：用户级的程序目录，可以理解为 C:/Progrem Files/，用户自己编译的软件默认会安装到这个目录下
  - `/usr/include`：存放了系统编程是所需的头文件
- `/var`：var 是 Variable 的缩写，这个目录中存放着在不断扩充着的东西，我们习惯将那些经常被修改的目录放在这个目录下，包括各种日志文件
- `/run`：是一个临时文件系统，存储系统启动以来的信息。当系统重启时，这个目录下的文件应该被删掉或清除

#### inode

![disk inode](./img/disk inode.png)

inode 结构体包含文件的属性信息有以下内容：

- 文件的字节数

- 文件拥有者的 id
- 文件所属组 id
- 文件的读写执行权限
- 文件的时间戳，共有三个：
  - ctime 指inode上一次变动的时间
  - mtime 指文件内容上一次变动时间
  - atime 指文件上一次打开时间
- 硬链接数，即有多少个文件指向这个 inode
- 文件数据块（block）的位置，即指向数据块的指针，包括一级指针，二级指针和三级指针，一般为15个指针

inode 中没有记载文件名和 inode 编号。inode 也会消耗硬盘空间，所以硬盘格式化的时候，操作系统自动将硬盘划分为两个区域：一个是数据区，存放文件数据，另一个是 inode 区（inode table），存放 inode 所包含的信息，inode 区本质上是一个结构体数组，数组下标就是 inode 编号`inode_num`

![inode](./img/inode.jpg)

当系统在找一个文件时，步骤如下：

- 通过文件名先找到文件的 inode 编号（数组下标）

- 通过 inode 编号找到文件 inode 结构体获取文件信息
- 通过 inode 结构体中的 block 指针找到文件内容

### stat

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *path, struct stat *buf);
```

- 此三个系统调用用于获取文件属性信息
- `stat`和`fstat`获取文件信息并写入缓冲区 buf，对于软链接文件则获取软链接指向的原文件的信息
- `lstat`对于软链接文件则获取此文件本身信息，其余与上面一致
- 成功执行返回 0，失败返回 -1 并设置 errno

```c
struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* Inode number */
    mode_t    st_mode;        /* File type and mode */
    nlink_t   st_nlink;       /* Number of hard links */
    uid_t     st_uid;         /* User ID of owner */
    gid_t     st_gid;         /* Group ID of owner */
    dev_t     st_rdev;        /* Device ID (if special file) */
    off_t     st_size;        /* Total size, in bytes */
    blksize_t st_blksize;     /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */

    /* Since Linux 2.6, the kernel supports nanosecond
       precision for the following timestamp fields.
       For the details before Linux 2.6, see NOTES. */

    struct timespec st_atim;  /* Time of last access */
    struct timespec st_mtim;  /* Time of last modification */
    struct timespec st_ctim;  /* Time of last status change */

#define st_atime st_atim.tv_sec      /* Backward compatibility */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
};
```



## 进程控制

## 信号

## 线程

## 高级 I/O

## 进程间通信

## 网络套接字

