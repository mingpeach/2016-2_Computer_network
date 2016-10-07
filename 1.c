#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 47500
#define MAX_LINE 20

int main(void)
{
	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char host[]="127.0.0.1";
	char buf[]="2013210128";
	int s;
	int len;

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "unknown host: %s\n",host);
		exit(1);
	}

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	/* active open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		close(s);
		exit(1);
	}

	/* main loop: get and send lines of text */
	len = strlen(buf) + 1;
	send(s, buf, len, 0);

	close(s);
}