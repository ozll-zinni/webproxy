/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,char *longmsg);
void echo(int connfd);

int main(int argc, char **argv) {
  int listenfd, connfd; // 서버 및 클라이언트 소켓 파일 디스크립터
  char hostname[MAXLINE], port[MAXLINE]; // 클라이언트 호스트네임 및 포트번호
  socklen_t clientlen; // 클라이언트 주소 구조체 크기
  struct sockaddr_storage clientaddr; // 클라이언트 주소 구조체


  /* Check command line args */
  //명령 인수 확인
  if (argc != 2) { // 인수 개수가 2가 아니면 오류 메시지 출력
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 클라이언트 연결 수신 소켓 생성
  // 주어진 포트번호에서  클라이언트의 연결 요청을 수신할 수 있는 소켓을 생성하고,
  // 그 소켓의 파일 디스크립터를 listenfd에 저장
    // 소켓 파일 디스크립터
      // 1. 서버 소켓 파일 디스크립터 (ㅣistenfd) : 서버가 클라이언트의 연결을 수신하는 소켓을 식별하는데 사용
      // 2. 클라이언트 소켓 파일 디스크립터 (connfd) : 서버가 클라이언트와 통신하기 위해 생성하는 각각의 소켓을 식별하는데 사용 -> 클라이언트가 수신해야 생성, 통신 담당
  listenfd = Open_listenfd(argv[1]); 
  // 클라이언트 요청 수락 및 처리
  while (1) {
    clientlen = sizeof(clientaddr); // 클라이언트 주소 구조체 크기 설정
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 수락 // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 클라이언트 호스트네임 및 포트번호 추출
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 연결 확인 메시지 출력
    doit(connfd); // 클라이언트 요청 처리 함수 호출  // line:netp:tiny:doit
    Close(connfd); // 연결 종료 및 소켓 닫음 // line:netp:tiny:close
  }
}

void echo(int connfd) {
  size_t n;
  char buf[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, connfd);
  while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
    if (strcmp(buf, "\r\n") == 0)
      break;
    Rio_writen(connfd, buf, n);
  }
}

/* doit -> 한 개의 HTTP 트랜잭션을 처리함 */
// 요청을 읽고, 해당하는 작업(동적/정적)을 수행하여 클라이언트에게 응답 보내기
void doit(int fd)
{
  int is_static; // 정적 파일 여부를 나타내는 플래그
  struct stat sbuf; // 파일의 상태 정보를 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 버퍼 및 요청 라인 구성 요소
  char filename[MAXLINE], cgiargs[MAXLINE]; // 요청된 파일의 경로 및 CGI 인자
  rio_t rio; // 리오 버퍼

  /* Read request line and headers */
  // Rio_readlineb ~sscanf -> 요청라인 읽고 분석
  /* 클라이언트가 rio로 보낸 요청 라인 분석 */
  Rio_readinitb(&rio, fd);                          //  리오 버퍼를 특정 파일 디스크립터(fd=>connfd)와 연결
  Rio_readlineb(&rio, buf, MAXLINE);                // rio(connfd)에 있는 string 한 줄(Request line)을 모두 buf로 옮긴다.
  printf("Request headers:\n");
  printf("%s", buf);                                // GET /index.html HTTP/1.1
  sscanf(buf, "%s %s %s", method, uri, version); 
  // method: "GET"
	// uri: "/index.html"
	// version: "HTTP/1.1"

  // tiny는 get요청만 받기 때문에, post요청이 들어오면 애러 메시지를 보내고 main루틴으로 돌아와서 연결을 닫고 다음 연결을 기다린다
  /* HTTP 요청의 메서드가 "GET"가 아닌 경우에 501 오류를 클라이언트에게 반환 */
  if (strcasecmp(method, "GET"))
  { // 조건문에서 하나라도 0이면 0
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  /* read_requesthdrs: 요청 헤더를 읽어들여서 출력 */
  /* 헤더를 읽는 것은 클라이언트의 요청을 처리하고 응답하기 위해 필요한 전처리 작업의 일부 */
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  // URI를 파일 이름과 비어 있을 수도 있는 CGI 인자 스트링으로 분석하고,
  // 요청이 정적 또는 동적 컨텐츠를 위한것인지 나타내는 플래그를 설정한다
  /* parse_uri: 클라이언트 요청 라인에서 받아온 uri를 이용해 정적/동적 컨텐츠를 구분한다.*/
  /* is_static이 1이면 정적 컨텐츠, 0이면 동적 컨텐츠 */
  is_static = parse_uri(uri, filename, cgiargs);

  // 파일이 디스크 상에 있지 않으면, 에러 메시지를 즉시 클라이언트에게 보내고 리턴한다
  /* filename: 클라이언트가 요청한 파일의 경로 */
  if (stat(filename, &sbuf) < 0) 
  { // 파일이 없다면 -1, 404에러
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) { /* Serve static content */
  // 만일 요청이 정적 컨텐츠를 위한 것이라면,
  // 이 파일이 보통 파일이라는 것과 읽기 권한을 가지고 있는지 검증한다
    // !(일반 파일이다) or !(읽기 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size); // 맞으면 정적 컨텐츠를 클라이언트에게 제공한다
  }
  else { /* Serve dynamic content */
  // 요청이 동적 컨텐츠에 대한 것이라면
  // 이 파일이 실행 가능한지 검증한다
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny coundn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs); //맞다면 진행해서 동적 컨텐츠를 제공한다
  }
}

/* clienterror -> 에러 메시지를 클라이언트에게 보냄 */
// HTTP 응답을 응답 라인에 적절한 상태 코드와 상태 메시지와 함께 클라이언트에게 보냄
// 브라우저 사용자에게 에러를 설명하는 응답 본체에 HTML 파일도 함께 보냄
// HTML(컨텐츠의 크기 + 타입 포함) 컨텐츠 한 개의 스트링으로 만듦 -> 크기를 쉽게 결정 할 수 있음
void clienterror(int fd, char *cause, char *errnum,
                  char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  /* HTTP 응답 본문 생성 */
  // sprintf(body, "<html><title>Tiny Error</title>");
  // sprintf(body, "%s<body bgcolor="
  //               "ffffff"
  //               ">\r\n",
  //         body);
  // sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  // sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  // sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);
  snprintf(body, sizeof(body), "<html><title>Tiny Error</title>");
  snprintf(body + strlen(body), sizeof(body) - strlen(body), "<body bgcolor=ffffff>\r\n");
  snprintf(body + strlen(body), sizeof(body) - strlen(body), "%s: %s\r\n", errnum, shortmsg);
  snprintf(body + strlen(body), sizeof(body) - strlen(body), "<p>%s: %s\r\n", longmsg, cause);
  snprintf(body + strlen(body), sizeof(body) - strlen(body), "<hr><em>The Tiny Web Server</em>\r\n");


  /* Print the HTTP response */
  /* HTTP 응답 헤더 출력 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  /* 에러 메시지와 응답 본체를 클라이언트에게 전송 */
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/* read_requesthdrs -> 요청 헤더를 읽고 무시한다 */
// Tiny는 요청 헤더 내의 어떤 정보도 사용하지 않음 -> read_requesthdrs함수를 호출해 이들을 읽고 무시함
void read_requesthdrs(rio_t *rp) {

  char buf[MAXLINE]; // 버퍼
  Rio_readlineb(rp, buf, MAXLINE); // 요청 라인을 읽어들여 무시한다. (요청 헤더부터 읽기 시작)

  // "\r\n"이 나올 때까지 요청 헤더를 읽어들여 화면에 출력
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE); // 한 줄을 읽는다.
    printf("%s", buf); // 읽어들인 헤더를 화면에 출력
  }
  return;
}

/* parse_uri -> HTTP URI를 분석한다 */
// 지역 파일의 내용을 포함하고 있는 본체를 갖는 HTTP 응답을 보낸다.
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // 정적 컨텐츠를 위한 경우
  if (!strstr(uri, "cgi-bin")) { // cgi-bin : 실행파일의 홈 디렉토리
    strcpy(cgiargs, ""); // cgi 인자 스트링 제거
    // ./index.html처럼 상대 리눅스 경로 이름으로 변경
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/') // /로 끝나면
      strcat(filename, "home.html"); // 기본 파일 이름을 추가한다 home.html : 기본 파일
    return 1;
  }
  // 동적 컨텐츠를 위한 경우
  else {
    // 모든 cgi 인자를 추출한다
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    // 나머지 uri 상대 리눅스 파일 이름으로 변경
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

/* serve_static -> 정적 컨텐츠를 클라이언트에게 서비스한다 */
//serve_static : 지역 파일의 내용 포함하는 본체를 갖는 HTTP 응답 보내기
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;                // 파일 디스크립터
  char *srcp,               // 파일 내용을 메모리에 매핑한 포인터
       filetype[MAXLINE],   // 파일의 MIME 타입
       buf[MAXBUF];         // 응답 헤더를 저장할 버퍼

  /* Semd response headers to client */
  /* 응답 헤더 생성 및 전송 */
  // get_filetype(filename, filetype);                         // 파일 타입 결정
  // sprintf(buf, "HTTP/1.0 200 OK\r\n");                      // 응답 라인 작성
  // // 응답 헤더
  // sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);       // 서버 정보 추가
  // sprintf(buf, "%sConnections: close\r\n", buf);            // 연결 종료 정보 추가
  // sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);  // 컨텐츠 길이 추가
  // sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // 컨텐츠 타입 추가

  get_filetype(filename, filetype);
  snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\r\n");
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Server: Tiny Web Server\r\n");
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Connection: close\r\n");
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-length: %d\r\n", filesize);
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "Content-type: %s\r\n\r\n", filetype);


  /* 응답 라인과 헤더를 클라이언트에게 보냄 */
  Rio_writen(fd, buf, strlen(buf)); 
  printf("Response headers: \n");
  printf("%s", buf);

  /* Send response body to client */
  /* 응답 바디 전송 */
  srcfd = Open(filename, O_RDONLY, 0);                       // 파일 열기
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일을 메모리에 동적할당
  /* Homework 11.9 */
  srcp = (char *) malloc(filesize);
  rio_readn(srcfd, srcp, filesize);

  Close(srcfd);                                              // 파일 닫기
  Rio_writen(fd, srcp, filesize);                           // 클라이언트에게 파일 내용 전송
  // Munmap(srcp, filesize);                                   // 메모리 할당 해제
  free(srcp);
}

 /* get_filetype -> Derive file type from filename */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");

  // Homework 11.7: MP4 비디오 타입 추가
  else if (strstr(filename, ".mpg"))
    strcpy(filetype, "video/mpg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

/* serve dynamic -> 동적 컨텐츠를 클라이언트에게 제공한다 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{ 
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  /* 클라이언트에 HTTP 응답 라인과 헤더를 전송 */
  sprintf(buf, "HTTP/1.1 200 OK\r\n"); // HTTP 응답 라인 생성
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에 응답 라인 전송
  sprintf(buf, "Server: Tiny Web Server\r\n"); // 서버 정보를 응답 헤더에 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트에 응답 헤더 전송

	/* CGI 실행을 위해 자식 프로세스를 생성 */
  if (Fork() == 0) // fork() 자식 프로세스 생성됐으면 0을 반환 (성공)
  { 
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // CGI 프로그램에 필요한 환경 변수 설정
    Dup2(fd, STDOUT_FILENO); // 자식 프로세스의 표준 출력을 클라이언트로 리다이렉션
    Execve(filename, emptylist, environ); // CGI 프로그램 실행
  }

  Wait(NULL); // 부모 프로세스가 자식 프로세스의 종료를 대기
}
