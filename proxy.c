#include <stdio.h>
#include "csapp.h"


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void proxy(int clientfd);
void read_requesthdrs(rio_t *rp);
void p_parse_uri(char *uri, char *hostname, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
  int listenfd, connfd; // 변수 선언
  char hostname[MAXLINE], port[MAXLINE]; // 문자열 변수 선언
  socklen_t clientlen; // 클라이언트 주소 길이 변수
  struct sockaddr_storage clientaddr; // 클라이언트 주소 구조체
  /* Check command line args */
  if (argc != 2) // 명령행 인자 개수 확인
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // 사용법 출력
    exit(1); // 프로그램 종료
  }
  listenfd = Open_listenfd(argv[1]); // 지정된 포트로 소켓 열기
  while (1) // 무한 루프
  {
    clientlen = sizeof(clientaddr); // 클라이언트 주소 길이 설정
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 클라이언트 호스트명 및 포트 추출
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 연결 수락 메시지 출력
    proxy(connfd); // 연결 처리 함수 호출
    Close(connfd); // 클라이언트 연결 닫기
  }
}


// 요청을 읽고, 해당하는 작업(동적/정적)을 수행하여 클라이언트에게 응답 보내기
void proxy(int clientfd)
{
    int serverfd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], request_header[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    char ip[MAXLINE], port[MAXLINE], hostname[MAXLINE], path[MAXLINE], header[MAXLINE];
    rio_t rio;
    int len;
    
    Rio_readinitb(&rio, clientfd); // 클라이언트 소켓과 rio 버퍼 연결
    Rio_readlineb(&rio, request_header, MAXLINE); // 클라이언트 요청 헤더 읽기
    sscanf(request_header, "%s %s", method, uri); // 요청에서 method와 uri 추출

    // URI 파싱
    p_parse_uri(uri, hostname, path, port);

    // 파싱 결과 출력 (디버깅용)
    printf("Hostname: %s, Port: %s, Path: %s\n", hostname, port, path);

    // 나머지 코드 계속 진행
    sprintf(request_header, "%s /%s %s\r\n", method, path, "HTTP/1.0");
    sprintf(request_header, "%sConnection: close\r\n", request_header);
    sprintf(request_header, "%sProxy-Connection: close\r\n", request_header);
    sprintf(request_header, "%s%s\r\n", request_header, user_agent_hdr);

    printf("%s", request_header); // 요청 헤더 출력

    serverfd = Open_clientfd(hostname, port); // 서버에 연결 시도
    if (serverfd < 0) {
        printf("Error connecting to server %s:%s\n", hostname, port);
        return;
    }

    // 요청 헤더 서버로 전송
    Rio_writen(serverfd, request_header, strlen(request_header));

    // 서버 응답 읽고 클라이언트로 전송
    ssize_t n = Rio_readn(serverfd, buf, MAX_OBJECT_SIZE);
    Rio_writen(clientfd, buf, n);

    Close(serverfd); // 서버 연결 닫기
}

/* clienterror -> 에러 메시지를 클라이언트에게 보냄 */
// HTTP 응답을 응답 라인에 적절한 상태 코드와 상태 메시지와 함께 클라이언트에게 보냄
// 브라우저 사용자에게 에러를 설명하는 응답 본체에 HTML 파일도 함께 보냄
// HTML(컨텐츠의 크기 + 타입 포함) 컨텐츠 한 개의 스트링으로 만듦 -> 크기를 쉽게 결정 할 수 있음
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  // 에러 메시지 생성
  sprintf(body, "<html><title>Proxy Error</title>"); // HTML 페이지의 시작 부분 작성
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); // 페이지 바탕색 설정
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg); // 에러 번호와 짧은 메시지 추가
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause); // 에러 원인 추가
  sprintf(body, "%s<hr><em>The Proxy server</em>\r\n", body); // 서버 정보 추가
  
  // HTTP 응답 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // HTTP 응답 라인 작성 (상태 코드와 메시지)
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에게 전송
  sprintf(buf, "Content-type: text/html\r\n"); // HTML 컨텐츠 타입 설정
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에게 전송
  sprintf(buf, "Content-length: %lu\r\n\r\n", strlen(body)); // HTML 본문의 길이 설정
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에게 전송
  Rio_writen(fd, body, strlen(body)); // HTML 본문을 클라이언트에게 전송
}

void p_read_requesthdrs(rio_t *rp, char *hostname, char *port, char *request_header)
{
  char buf[MAXLINE]; // 버퍼 선언
  Rio_readlineb(rp, buf, MAXLINE); // 첫 번째 헤더 읽기

  while (strcmp(buf, "\r\n")) // 빈 줄이 나올 때까지 반복
  {
    if (strstr(buf, "User-Agent:") == buf) // User-Agent 헤더인 경우
    {
      char user_agent_buf[MAXLINE]; // User-Agent 헤더를 저장할 버퍼
      snprintf(user_agent_buf, MAXLINE, user_agent_hdr); // 사용자 에이전트 헤더 복사
      strcpy(request_header, user_agent_buf); // 요청 헤더에 사용자 에이전트 헤더 복사
    }
    Rio_readlineb(rp, buf, MAXLINE); // 다음 헤더 읽기
    strcat(request_header, buf); // 읽은 헤더를 요청 헤더에 추가
  }
  return; // 함수 종료
}

void p_parse_uri(char *uri, char *hostname, char *path, char *port)
{
    char *ptr = strstr(uri, "//");
    ptr = (ptr != NULL) ? ptr + 2 : uri;  // "http://" 또는 "https://"를 건너뜀

    // 호스트 이름 추출
    char *host_end = strchr(ptr, ':');
    if (host_end == NULL) {
        host_end = strchr(ptr, '/');
    }

    if (host_end == NULL) {
        strcpy(hostname, ptr);  // '/'도 없는 경우 전체를 호스트 이름으로 간주
        strcpy(port, "80");     // 기본 포트로 80 설정
        strcpy(path, "/");      // 기본 경로로 '/' 설정
        return;
    }

    int len = host_end - ptr;
    strncpy(hostname, ptr, len);
    hostname[len] = '\0';

    if (*host_end == ':') {
        // 포트 번호 추출
        char *port_start = host_end + 1;
        char *path_start = strchr(port_start, '/');
        if (path_start == NULL) {
            strcpy(port, port_start);  // 경로가 없으면 포트만 설정
            strcpy(path, "/");         // 경로가 없는 경우 기본 경로 설정
        } else {
            len = path_start - port_start;
            strncpy(port, port_start, len);
            port[len] = '\0';
            strcpy(path, path_start);  // 경로가 있는 경우 경로 복사
        }
    } else {
        // 포트 번호가 없는 경우 기본 포트 80 사용
        strcpy(port, "80");
        strcpy(path, host_end);  // '/' 이후는 경로로 설정
    }
}
