#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <errno.h>

class http_conn{
public:
    static const int FILENAME_LEN = 200;        // 文件名的最大长度
    static const int READ_BUFFER_SIZE = 1024;   // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小

    // HTTP请求方法，这里只支持GET
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};

    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };

    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };

    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn(){};
    ~http_conn(){};

public:
    // initialize new connection
    void init(int sockfd, const sockaddr_in& adr);

    // NIO read
    bool read_data();
    
    // NIO write
    bool write();
    
    // Process client requests
    void process();

    // close http connection
    void close_conn();
private:
    void init();
    HTTP_CODE process_read();
    bool process_write( HTTP_CODE ret );    // 填充HTTP应答

    // 下面这一组函数被process_read调用以分析HTTP请求
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    // 这一组函数被process_write调用以填充HTTP应答。
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_content_type();
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    // all events on the socket are registered to the same epoll kernel event, so it is set to static
    static int m_epollfd;

    // counting the number of users
    static int m_user_count;

private:
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;

    CHECK_STATE m_check_state;              // 主状态机当前所处的状态
    METHOD m_method;                        // 请求方法

    char m_real_file[ FILENAME_LEN ];       // 客户请求的目标文件的完整路径，其内容等于 doc_root + m_url, doc_root是网站根目录
    char* m_url;                            // 客户请求的目标文件的文件名
    char* m_version;                        // HTTP协议版本号，我们仅支持HTTP1.1
    char* m_host;                           // 主机名
    int m_content_length;                   // HTTP请求的消息总长度
    bool m_linger;                          // HTTP请求是否要求保持连接

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    char* m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;

};


#endif