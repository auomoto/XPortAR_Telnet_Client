#include <stdio.h>			// for fflush(stdout);
#include <errno.h>
#include <stdlib.h>			// for exit();
#include <arpa/inet.h>			// for sockets
#include <fcntl.h>
//#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#define XPORTIP		"192.168.1.99"	// Address on home network
#define TELNETPORT	23
#define BUFSIZE		512
#define MINPOS		55		// Rotation range limits for my servo motor
#define MAXPOS		243		// Values are 100X pulse width in msec

int initXPort(int);			// Function prototypes
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

	if (XPortSock < 0) {			// Couldn't connect to XPortAR
		fprintf(stderr, "main(): telnetToXPort() error, exiting\n");
		fflush(stderr);
		exit(0);
	}

	if (initXPort(XPortSock) < 0) {		// Couldn't set up for telnet
		fprintf(stderr, "main(): initXPort() error, exiting\n");
		fflush(stderr);
		exit(0);
	}

	moveTo(XPortSock, 55);			// Opening flourish
	usleep(750000);
	moveTo(XPortSock, 243);
	usleep(750000);
	moveTo(XPortSock, 55);
	usleep(750000);
	moveTo(XPortSock, 243);
	usleep(750000);
	moveTo(XPortSock, 150);
	usleep(2000000);

	srand(time(NULL));
	for (i = 0; i < 100; i++) {		// Move motor randomly
		position = (rand() % (244-55)) + 55;
		printf("%3d ", i);
		moveTo(XPortSock, position);
		usleep(750000);
	}

	moveTo(XPortSock, 55);			// Closing
	usleep(750000);
	moveTo(XPortSock, 243);
	usleep(750000);
	moveTo(XPortSock, 55);
	usleep(750000);
	moveTo(XPortSock, 243);
	usleep(750000);
	moveTo(XPortSock, 150);
	usleep(2000000);

	exit(0);

}

/*---------------------------------------------------------------------------

	Sends a command (an ascii number string between 55 and 243) to the
	ATtiny servo board (see the github repository for ATtiny4313_Servo).
	That servo motor has a range of pulse widths from 0.55 ms to 2.43 ms
	and the number sent to the ATtiny4313 is 100X the pulse width in ms.

---------------------------------------------------------------------------*/
void moveTo(fd, value)
int fd, value;
{

	char buf[BUFSIZE];

	if (value < MINPOS) value = MINPOS;
	if (value > MAXPOS) value = MAXPOS;
	sprintf(buf, "%d\r", value);		// ATtiny4313_Servo program wants
	writeXPort(fd, buf);			// a number followed by a '\r'.
	readXPort(fd, buf, '>');		// Clear the ATtiny reply

}

/*---------------------------------------------------------------------------

	Sends commands to the XPort AR to connect to serial line 1 by telnet.
	After the initial connection and receipt of the '>' prompt), the
	XPort AR wants two commands: "enable" and "connect line 1":

		>enable
		(enable)#connect line 1

	The connect command reply is the typical telnet notice stating that
	<control>L is the escape character.

	The initial connection returns a stream of about 11 bytes before
	sending the '>' prompt character.

---------------------------------------------------------------------------*/
int initXPort(fd)
int fd;
{

	char buf[BUFSIZE];
	int n;

	n = readXPort(fd, buf, '>');		// Initial connection returns a '>'

	if (n < 0) {
		fprintf(stderr, "initXPort(): error on initial connect (no '>')\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): initial connect returns %s\n", buf);
		fflush(stdout);
	}

	memset(buf, 0, BUFSIZE);		// Send the "enable" command
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

	n = readXPort(fd, buf, '#');		// "(enable)#" is received
	if (n < 0) {
		fprintf(stderr, "initXPort(): error reading \"(enable)#\" command\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): read %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

	memset(buf, 0, BUFSIZE);		// send the "connect line 1" command
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

	n = readXPort(fd, buf, '\n');		// Receive telnet message
	if (n < 0) {
		fprintf(stderr, "initXPort(): error reading \"<control>L response\"\n");
		fflush(stderr);
		return(-1);
	} else {
		printf("initXPort(): read %d bytes: %s\n", n, buf);
		fflush(stdout);
	}

/*///////////////////////////////////////////////////////////////////////////////
	// Loopback test (Rx and Tx are connected on the chip/board)
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
///////////////////////////////////////////////////////////////////////////////*/

	return(0);

}


/*---------------------------------------------------------------------------

	Read the serial data through the XPort AR telnet connection until the
	endchar is seen.

	The XPort AR socket is non-blocking. Loop on a read until the expected
	ending character in the stream is detected or the loop count is exceeded.

---------------------------------------------------------------------------*/
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
			fprintf(stderr, "readXPort(): read() error: %s\n", strerror(errno));
			fflush(stderr);
			return(-1);
		} else {
			usleep(10000);
		}	
	}
	return(strlen(buf));

}

/*---------------------------------------------------------------------------

	Opens a non-blocking socket to the XPort AR.

---------------------------------------------------------------------------*/
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
		fprintf(stderr, "telnetToXPort(): inet_pton() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-1);
	}

	if ((XPortSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "telnetToXPort(): socket() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-2);
	}

	if (connect(XPortSock, (struct sockaddr *) &XPortSockAddr, sizeof(XPortSockAddr))) {
		fprintf(stderr, "telnetToXPort(): connect() error: %s\n", strerror(errno));
		fflush(stderr);
		return(-3);
	}

	i = fcntl(XPortSock, F_GETFL, 0);
	fcntl(XPortSock, F_SETFL, i | O_NONBLOCK);

	return(XPortSock);

}

/*---------------------------------------------------------------------------

	Writes to the serial port.

---------------------------------------------------------------------------*/
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
