// /*
//  * adder.c - a minimal CGI program that adds two numbers together
//  */
// /* $begin adder */
// #include "csapp.h"

// int main(void) {
//     char *buf, *p;
//     char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
//     int n1 = 0, n2 = 0;

//     /* Extract the two arguments */
//     /* 두 개의 인자 추출 */
//     if ((buf = getenv("QUERY_STRING")) != NULL) {
//         p = strchr(buf, '&');
//         *p = '\0';
//         strcpy(arg1, buf);
//         strcpy(arg2, p + 1);
//         n1 = atoi(arg1); // 첫 번째 인자를 정수로 변환
//         n2 = atoi(arg2); // 두 번째 인자를 정수로 변환
//     }

//     /* Make the response body */
//     /* 응답 본문 생성 */
//     sprintf(content, "QUERY_STRING=%s", buf);
//     sprintf(content, "Welcome to add.com: ");
//     sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
//     sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
//             content, n1, n2, n1 + n2);
//     sprintf(content, "%sThanks for visiting!\r\n", content);

//     /* Generate the HTTP response */
//     /* HTTP 응답 생성 */
//     printf("Connection: close\r\n");
//     printf("Content-length: %d\r\n", (int) strlen(content));
//     printf("Content-type: text/html\r\n\r\n");
//     printf("%s", content); // 생성된 응답 본문 출력
//     fflush(stdout); // 출력 버퍼를 비워줌

//     exit(0);
// }
// /* $end adder */

/*
 * adder.c - 두 숫자를 더하는 최소한의 CGI 프로그램
 */
/* $begin adder */
#include "csapp.h"  // 필요한 라이브러리를 포함시킴 (csapp는 시스템 호출과 I/O 관련 기능을 포함한 라이브러리)

/* 메인 함수 */
int main(void) {
  char *buf, *p;  // 문자열을 저장할 포인터 변수들 (buf: 쿼리 스트링을 저장, p: '&'를 찾은 위치 저장)
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];  // 각각 첫 번째, 두 번째 숫자를 저장할 문자열과 응답 내용을 저장할 배열
  int n1=0, n2=0;  // 두 개의 정수를 저장할 변수 (초기값 0)

  /* 두 개의 인자(숫자)를 추출하는 부분 */
  if ((buf = getenv("QUERY_STRING")) != NULL){  // 환경 변수 QUERY_STRING에서 데이터를 가져옴 (웹 브라우저에서 전달된 쿼리 스트링)
    p = strchr(buf, '&');  // 쿼리 스트링에서 '&' 문자를 찾아 위치를 p에 저장 (두 숫자를 구분하기 위한 문자)
    *p = '\0';  // '&' 문자를 NULL 문자로 바꿔 첫 번째 숫자와 두 번째 숫자를 분리≠
    strcpy(arg1, buf);  // 첫 번째 숫자를 arg1에 복사
    strcpy(arg2, p+1);  // '&' 이후의 두 번째 숫자를 arg2에 복사
    n1 = atoi(arg1);  // 첫 번째 숫자 문자열을 정수로 변환하여 n1에 저장
    n2 = atoi(arg2);  // 두 번째 숫자 문자열을 정수로 변환하여 n2에 저장
  }

  /* 응답 본문을 만드는 부분 */
  sprintf(content, "QUERY_STRING=%s", buf);  // 쿼리 스트링을 content에 저장 (디버깅 용도로 보통 사용)
  sprintf(content, "Welcome to add.com: ");  // content에 환영 메시지를 저장
  sprintf(content, "%sTHE Internet addiction portal. \t\n<p>", content);  // 더 긴 메시지를 content에 덧붙임
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",  // 계산 결과를 content에 추가
          content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);  // 마지막으로 방문 감사 메시지를 content에 추가

  /* HTTP 응답을 생성하는 부분 */
  printf("Connection: close\r\n");  // 연결을 닫는다는 HTTP 헤더를 출력
  printf("Content-length: %d\r\n", (int)strlen(content));  // 응답 본문의 길이를 출력
  printf("Content-type: text/html\r\n\r\n");  // 응답의 타입이 HTML이라는 것을 알림
  printf("%s", content);  // 실제로 응답 본문을 출력 (content에 저장된 HTML 코드를 출력)
  fflush(stdout);  // 출력 버퍼를 강제로 비움 (즉시 클라이언트로 전송)

  exit(0);  // 프로그램 종료 (정상 종료)
}
/* $end adder */