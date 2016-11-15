#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 47500

/* Flag */
#define FLAG_HELLO ((unsigned char)(0x01 << 7))
#define FLAG_INSTRUCTION ((unsigned char)(0x01 << 6))
#define FLAG_RESPONSE ((unsigned char)(0x01 << 5))
#define FLAG_TERMINATE ((unsigned char)(0x01 << 4))

/* OP type */
#define OP_ECHO_REQUEST ((unsigned char)(0x00))
#define OP_INCREMENT ((unsigned char)(0x01))
#define OP_DECREMENT ((unsigned char)(0x02))

struct header
{
	unsigned char flags;
	unsigned char op;
	unsigned short seq_num;
	unsigned int data_len;
};	

int main(void)
{
	struct hostent *hp;
	struct sockaddr_in sin;
	char host[]="127.0.0.1";
	int s; // socket file descriptor

	char buf[1024]; 
	char buf_get[1024]; 
	char data_str[1024]; 
	int data_int; 
	long long * data_long;
	long long tmp;
	int len;
	data_long=&tmp;
	int i;

	unsigned char flags_get[2];
	unsigned char op_get[2];
	unsigned short seq_num_get;
	unsigned int data_len_get;
	
	/* translate host name into peer's IP address */
	hp = gethostbyname(host); // IP 주소를 이용해 hostent 정보 구함
	if (!hp) { // hostent 정보 구할 수 없으면
		fprintf(stderr, "unknown host: %s\n",host);
		exit(1);
	}

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin)); // sin을 0으로 채운다.
	sin.sin_family = AF_INET; // AF_INET : 주소 체계 중 IPv4 인터넷 프로토콜을 의미
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length); //hp의 주소를 sin 주소에 복사
	sin.sin_port = htons(SERVER_PORT); // port 설정

	/* --소켓 생성-- */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) { 
	// PF_INET : 프로토콜 체계 중 IPv4 인터넷 프로토콜
	// SOCK_STEAM : 연결 지향형 소켓
		perror("socket");
		exit(1);
	}

	/* --연결요청-- */
	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		close(s);
		exit(1);
	}	

	/* --header 생성-- */
	struct header *header_p;
	header_p = (struct header *)malloc(sizeof(struct header));

	/* --HELLO message-- */
	header_p -> flags = FLAG_HELLO;
	header_p -> op = OP_ECHO_REQUEST;
	header_p -> seq_num = htons((unsigned short)0); /* 비트수 맞추기 */
	header_p -> data_len = htonl(4);

	*data_long = htonl(2013210128);

	memcpy(buf, header_p, sizeof(struct header)); /* header 복사 */
	memcpy(buf + sizeof(struct header), data_long, 4); /* data 복사 */

	/* connection 시작하는 부분 */
	len = sizeof(struct header) + sizeof(data_long) + 1;
	send(s, buf, len, 0);

	printf("*** starting ***\n\n");
	printf("sending first hello msg..\n");

	/* 읽어오기 */
	while(1)
	{
		memset(buf_get,0,sizeof(buf_get));
		read(s, buf_get, 1024);

		memset(flags_get,0,sizeof(unsigned char));
		memcpy(flags_get,buf_get,1);

		memset(op_get,0,sizeof(unsigned char));
		memcpy(op_get,buf_get+1,1);

		if(flags_get[0]==FLAG_HELLO)
		{
			printf("received a hello message from the server! \n waiting for the first instruction message...\n");
		}	

		else if(flags_get[0]==FLAG_INSTRUCTION)
		{
			seq_num_get = ntohs(*(unsigned short *)(buf_get+2));
			data_len_get = ntohl(*(int *)(buf_get+4));
			printf("received an instruction message! received data_len : %d bytes\n",data_len_get);

			if(op_get[0]==OP_ECHO_REQUEST) // ECHO request
			{
				//받은 문자열 그대로 출력
				printf("op type is echo request!\n");

				memset(data_str,0,strlen(data_str));
				for(i=0;i<data_len_get;i++)
				{
					data_str[i]=(buf_get+sizeof(struct header))[i];
				}

				header_p -> flags = FLAG_RESPONSE;
				header_p -> op = OP_ECHO_REQUEST;
				header_p -> seq_num = htons(seq_num_get); 
				header_p -> data_len = htonl(data_len_get);

				memset(buf,0,sizeof(buf));
				memcpy(buf, header_p, sizeof(struct header)); // header 복사 
				memcpy(buf + sizeof(struct header), data_str, strlen(data_str)+1); // data 복사 
				len = sizeof(struct header) + data_len_get + 1;
				send(s, buf, len, 0);

				printf("sent response message with seqence number %d to server\n\n\n",seq_num_get);

			}
			else if(op_get[0]==OP_INCREMENT) // INCREMENT message
			{
				data_int = ntohl(*(int *)(buf_get+sizeof(struct header)));				
				printf("op type is increment!\n");

				data_int = htonl(++data_int);
	
				header_p -> flags = FLAG_RESPONSE;
				header_p -> op = OP_ECHO_REQUEST;
				header_p -> seq_num = htons(seq_num_get); 
				header_p -> data_len = htonl(4);	

				memset(buf,0,sizeof(buf));
				memcpy(buf, header_p, sizeof(struct header)); // header 복사 				
				memcpy(buf + sizeof(struct header), &data_int, 4); // segmentation fault

				len = sizeof(struct header) + sizeof(data_int);
				send(s, buf, len, 0);			

				printf("sent response message with seqence number %d to server\n\n\n",seq_num_get);		
			}
			else // DECREMENT message
			{
				data_int = ntohl(*(int *)(buf_get+sizeof(struct header)));				
				printf("op type is decrement!\n");

				data_int = htonl(--data_int);

				header_p -> flags = FLAG_RESPONSE;
				header_p -> op = OP_ECHO_REQUEST;
				header_p -> seq_num = htons(seq_num_get);  
				header_p -> data_len = htonl(4);	

				memset(buf,0,sizeof(buf));
				memcpy(buf, header_p, sizeof(struct header)); // header 복사 
				memcpy(buf + sizeof(struct header), &data_int, 4); // segmentation fault

				len = sizeof(struct header) + sizeof(data_int);
				send(s, buf, len, 0);	

				printf("sent response message with seqence number %d to server\n\n\n",seq_num_get);							
			}
		}

		else if(flags_get[0]==FLAG_TERMINATE)
		{
			printf("received terminate msg! terminating...\n");
			close(s);
			return 0;
		}

	}

}