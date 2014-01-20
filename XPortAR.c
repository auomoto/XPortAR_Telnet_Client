#include <stdio.h>			// for fflush(stdout);
#include <errno.h>
#include <stdlib.h>			// for exit();
#include <arpa/inet.h>			// for sockets
#include <fcntl.h>
//#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#define XPORTIP	"192.168.1.99"		// Address on home network
#define TELNETPORT	23
#define BUFSIZE		512

#define MINPOS		55		// Special numbers for my servo motor
#define MAXPOS		243		// 100X ms of pulse width

int initXPort(int);
void moveTo(int, int);
int readXPort(int, char*, char);
int telnetToXPort(char *);
int writeXPort(int, char*);

int main(argv, argc)
int argv;
char *argc[];
{

	char ipaddress[17];
	int i, position, XPortSock;

	if (argv == 2) {			// IP address on the command line?
		strcpy(ipaddress, argc[1]);
	} else {
		strcpy(ipaddress, XPORTIP);
	}


	XPortSock = telnetToXPort(ipaddress);
	if (XPortSock < 0) {
		fprintf(stderr, "main(): telnetToXPort() error, exiting\n");
		fflush(stderr);
		exit(0);
	}

	if (initXPort(XPortSock) < 0) {
		fprintf(stderr, "main(): initXPort() error, exiting\n");
		fflush(stderr);
		exit(0);
	}

	for (i = 0; i < 200; i++) {
		position = (rand() % (243-55)) + 55;
		printf("%3d ", i);
		moveTo(XPortSock, position);
		usleep(750000);
	}

	exit(0);

}

void moveTo(fd, value)
int fd, value;
{

	char buf[BUFSIZE];

	sprintf(buf, "%d\r", value);
	writeXPort(fd, buf);
	readXPort(fd, buf, '>');

}

int initXPort(fd)
int fd;
{

	char buf[BUFSIZE];
	int n;

	// Initial connection eventually returns a '>' after a bunch of junk
	n = readXPort(fd, buf, '>');
	if (n < 0) {
		fprintf(stderr, "initXPort(): error on initial connect (no '>')\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): initial connect returns %s\n", buf);
		fflush(stdout);
	}

	// Write the XPortAR "enable" command
	memset(buf, 0, BUFSIZE);
	strcpy(buf, "enable\r");
	n = writeXPort(fd, buf);
	if (n < 0) {
		fprintf(stderr, "initXPort(): error writing \"enable\" command\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): wrote %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

	// The XPortAR "enable" command returns the string "(enable)#"
	n = readXPort(fd, buf, '#');
	if (n < 0) {
		fprintf(stderr, "initXPort(): error reading \"(enable)#\" command\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): read %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

	// Send the "connect line 1" command
	memset(buf, 0, BUFSIZE);
	strcpy(buf, "connect line 1\r");
	n = writeXPort(fd, buf);
	if (n < 0) {
		fprintf(stderr, "initXPort(): error writing \"connect line 1\" command\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): wrote %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

	// Read the telnet message about ^L escape
	n = readXPort(fd, buf, '\n');
	if (n < 0) {
		fprintf(stderr, "initXPort(): error reading \"<control>L response\"\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): read %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

/*
	// Loopback test (Rx and Tx must be connected on the chip/board)
	memset(buf, 0, BUFSIZE);
	strcpy(buf, "echotest\r");
	n = writeXPort(fd, buf);
	if (n < 0) {
		fprintf(stderr, "initXPort(): error writing \"echotest\"\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): wrote %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

	n = readXPort(fd, buf, '\r');
	if (n < 0) {
		fprintf(stderr, "initXPort(): error reading \"echotest\" echo\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): read %d bytes: %s\n", n, buf);
		fflush(stdout);
	}
*/

	return(0);

}


int readXPort(fd, buf, endchar)
int fd;
char *buf, endchar;
{

	char tbuf[BUFSIZE];
	int ncount, nread;

	ncount = 0;

	memset(buf, 0, BUFSIZE);

	ncount = 0;
	while (strrchr(buf, endchar) == NULL) {
		ncount++;
		if (ncount > 500) {
			fprintf(stderr, "readXPort(): timeout\n");
			fflush(stderr);
			return(-1);
		}
		memset(tbuf, 0, BUFSIZE);
		nread = read(fd, tbuf, BUFSIZE);
		if (nread > 0) {
			strcat(buf, tbuf);
		} else if ((nread < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
			fprintf(stderr, "readXPort(): read() error\n");
			fflush(stderr);
			return(-1);
		} else {
			usleep(10000);
		}	
	}
	return(strlen(buf));

}

int telnetToXPort(ipaddress)
char *ipaddress;
{

	char buf[BUFSIZE];
	int i, XPortSock;
	struct sockaddr_in XPortSockAddr;

	memset(buf, 0, BUFSIZE);
	memset((char *) &XPortSockAddr, 0, sizeof(XPortSockAddr));

	XPortSockAddr.sin_family = AF_INET;
	XPortSockAddr.sin_port = htons(TELNETPORT);

	if (inet_pton(AF_INET, ipaddress, &(XPortSockAddr.sin_addr)) <= 0) {
		fprintf(stderr, "telnetToXport(): inet_pton() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-1);
	}

	if ((XPortSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "telnetToXport(): socket() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-2);
	}

	if (connect(XPortSock, (struct sockaddr *) &XPortSockAddr, sizeof(XPortSockAddr))) {
		fprintf(stderr, "telnetToXport(): connect() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-3);
	}

	i = fcntl(XPortSock, F_GETFL, 0);
	fcntl(XPortSock, F_SETFL, i | O_NONBLOCK);

	return(XPortSock);

}

int writeXPort(fd, buf)
int fd;
char *buf;
{

	int n;

	n = write(fd, buf, strlen(buf));

	if (n < 0) {
		fprintf(stderr, "writeXPort(): write() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-1);
	}
	if (n != strlen(buf)) {
		fprintf(stderr, "writeXPort(): wrote %d bytes instead of %d bytes\n", n, strlen(buf));
		fflush(stderr);
		return(-1);
	}

	printf("writeXPort(): wrote %d bytes: %s\n", n, buf);
	fflush(stdout);

	return(n);

}
