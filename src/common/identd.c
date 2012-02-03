/* simple identd server for xchat under win32 */

/* Compile in Linux using:
g++ -pthread -I/usr/include -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/ -o identd identd.c

Notes on Linux porting:
- Threaded functions in Linux *MUST* have a "void *(*)(void *)" signature, meaning
  identd and identd_ipv6 must be changed from static int to void (or void*)

- For some reason, gcc is complaining if int main(){} is not explicitely declared

- For an unknown reason, compiling requires PrintText to be *implemented*
  (no, just #include "text.h" is NOT enough)

- CloseHandle was not ported, as I fail to understand killing a thread right after calling it,
  but the most similar function would be pthread_exit

- Only tested *without* IPv6 support (ie, USE_IPV6 is NOT defined)
*/

#define WANTSOCKET

#ifndef WIN32						/* for unix */
typedef unsigned long DWORD;
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "xchat.h"
#include "xchatc.h"
#include "inet.h"

static int identd_is_running = FALSE;
#ifdef USE_IPV6
static int identd_ipv6_is_running = FALSE;
#endif

#ifdef WIN32
static int
identd (char *username)
{
	int len;
#else						       /* for unix */
/* This is ugly, but... */
int main() {}
void PrintText (session *sess, char *text) { return; }
/* someone please find a way to change the above to:
#include "text.h"
*/

void *
identd (void *v_username)
{
	char * username = (char *) v_username;
	socklen_t len;
#endif
	int sok, read_sok;
	char *p;
	char buf[256];
	char outbuf[256];
	struct sockaddr_in addr;

	sok = socket (AF_INET, SOCK_STREAM, 0);
	if (sok == INVALID_SOCKET)
	{
		free (username);
		return 0;
	}

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons (113);

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		closesocket (sok);
		free (username);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		free (username);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);
	if (read_sok == INVALID_SOCKET)
	{
		free (username);
		return 0;
	}

	identd_is_running = FALSE;

	snprintf (outbuf, sizeof (outbuf), "%%\tServicing ident request from %s\n",
				 inet_ntoa (addr.sin_addr));
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');
	if (p)
	{
		snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n",
					 atoi (buf), atoi (p + 1), username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	free (username);

	return 0;
}

#ifdef USE_IPV6
static int
identd_ipv6 (char *username)
{
	int sok, read_sok, len;
	char *p;
	char buf[256];
	char outbuf[256];
	char ipv6buf[60];
	DWORD ipv6buflen = sizeof (ipv6buf);
	struct sockaddr_in6 addr;

	sok = socket (AF_INET6, SOCK_STREAM, 0);

	if (sok == INVALID_SOCKET)
	{
		free (username);
		return 0;
	}

	len = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &len, sizeof (len));

	memset (&addr, 0, sizeof (addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons (113);

	if (bind (sok, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		closesocket (sok);
		free (username);
		return 0;
	}

	if (listen (sok, 1) == SOCKET_ERROR)
	{
		closesocket (sok);
		free (username);
		return 0;
	}

	len = sizeof (addr);
	read_sok = accept (sok, (struct sockaddr *) &addr, &len);
	closesocket (sok);

	if (read_sok == INVALID_SOCKET)
	{
		free (username);
		return 0;
	}

	identd_ipv6_is_running = FALSE;

	if (WSAAddressToString ((struct sockaddr *) &addr, sizeof (addr), NULL, &ipv6buf, &ipv6buflen) == SOCKET_ERROR)
	{
		snprintf (ipv6buf, sizeof (ipv6buf) - 1, "[SOCKET ERROR: 0x%X]", WSAGetLastError ());
	}

	snprintf (outbuf, sizeof (outbuf), "%%\tServicing ident request from %s\n", ipv6buf);
	PrintText (current_sess, outbuf);

	recv (read_sok, buf, sizeof (buf) - 1, 0);
	buf[sizeof (buf) - 1] = 0;	  /* ensure null termination */

	p = strchr (buf, ',');

	if (p)
	{
		snprintf (outbuf, sizeof (outbuf) - 1, "%d, %d : USERID : UNIX : %s\r\n", atoi (buf), atoi (p + 1), username);
		outbuf[sizeof (outbuf) - 1] = 0;	/* ensure null termination */
		send (read_sok, outbuf, strlen (outbuf), 0);
	}

	sleep (1);
	closesocket (read_sok);
	free (username);

	return 0;
}
#endif

void
identd_start (char *username)
{
	DWORD tid;

#ifdef USE_IPV6
	DWORD tidv6;
	if (identd_ipv6_is_running == FALSE)
	{
		identd_ipv6_is_running = TRUE;
		CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) identd_ipv6,
						 strdup (username), 0, &tidv6));
	}
#endif

	if (identd_is_running == FALSE)
	{
		identd_is_running = TRUE;

#ifdef WIN32						/* for windows */
		CloseHandle (CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) identd,
						 strdup (username), 0, &tid));
#else						       /* for unix */
		pthread_create (&tid, NULL, identd, strdup (username));
		// pthread_exit (0); # What would be the point of creating a thread and kill it before it naturally ends?
#endif
	}
}
