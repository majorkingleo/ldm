/*
    LDM - a simple XDMCP Display Manager that allows you not to make your own
    choice if you wanna accept a request before a willing message is sent
    Copyright (C) 2004 by Martin Oberzalek kingleo@gmx.at 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <fcntl.h>

#include <list>

enum OPCODES
  {
    UNKNOWN         =  0,
    BROADCAST_QUERY =  1,
    QUERY           =  2,
    INDIRECT_QUERY  =  3,
    FORWARD_QUERY   =  4,
    WILLING         =  5,
    UNWILLING       =  6,
    REQUEST         =  7,
    ACCEPT          =  8,
    DECLINE         =  9,
    MANAGE          = 10,
    REFUSE          = 11,
    FAILED          = 12,
    ALIVE           = 13,
    KEEP_ALIVE      = 14
  };

const int SERVER_PORT = 177;

int session_id = 0;

std::list<pid_t> child_list;

int create_socket()
{
    struct sockaddr_in addresse;
    int                sockfd;
    int                i;
    
    if( (sockfd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
	perror( "creating socket failed" );
	return -1;
    }

    setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof( i ) );

    addresse.sin_family = AF_INET;
    addresse.sin_port = htons(SERVER_PORT);
    memset( &addresse.sin_addr, 0, sizeof( addresse.sin_addr ) );

    if( bind( sockfd, (struct sockaddr *) &addresse, sizeof( addresse ) ) ) {
      perror( "bind failed" );
      close( sockfd );
      return -2;
    }
    
    printf( "listening on localhost on udp port %d\n", SERVER_PORT );

    return sockfd;
}

int get_CARD8( char **buffer )
{
  int n = *(buffer[0]);

  *buffer++;

  return n;
}


int get_CARD16( char **buffer )
{
  int n;

  memcpy( &n, *buffer, 2 );

  *buffer += 2;

  return ntohs( n );
}

int get_CARD32( char **buffer )
{
  int n;

  memcpy( &n, *buffer, 4 );

  *buffer += 4;

  return ntohl( 4 );
}

void put_CARD16( char **buffer, int num, int *len )
{
  short n = htons( num );

  memcpy( *buffer, &n, 2 );
  *buffer += 2;
  if( len != NULL )
    *len += 2;
}

void put_CARD32( char **buffer, int num, int *len )
{
  int n = htonl( num );

  memcpy( *buffer, &n, 4 );
  *buffer += 4;
  if( len != NULL )
    *len += 4;
}

void put_CARD8( char **buffer, char n, int *len  )
{
  memcpy( *buffer, &n, 1 );
  *buffer += 1;
  if( len != NULL )
    *len += 1;
}

void do_query( char *message, int length, int sockfd, struct sockaddr_in addresse )
{
  char buffer[1024];
  char *buf = buffer;
  const char *usename = "";
  int authname_len = strlen( usename );
  char hostname[1024];
  int hostname_len = strlen( hostname );
  int len = 0;


  printf( "do_query()\n" );
  
  gethostname( hostname, sizeof( hostname ) );

  if( length > 0 )
    {
      int auth_num = get_CARD8( &message );
      int i;

      for( i = 0; i < auth_num; i++ )
	{
	  int size = get_CARD8( &message );
	  char auth[1024];
	  
	  memcpy( auth, message, size );
	  auth[size] = '\0';
	  message += size;

	  printf( "auth method available by the client: %s\n", auth );
	}

      if( auth_num )
	printf( "auth method available, we just don't have none implemented\n" );
      else
	printf( "no authmetod available by the client, cool we won't use any.\n" );
    }

  /* reply willing message */
  put_CARD16( &buf, 1, &len );
  put_CARD16( &buf, WILLING, &len );
  put_CARD16( &buf, authname_len + hostname_len + 2 + 6, &len );
  put_CARD16( &buf, authname_len, &len );

  memcpy( buf, usename, authname_len );
  buf += authname_len;
  len += authname_len;

  put_CARD16( &buf, hostname_len, &len );
  memcpy( buf, hostname, hostname_len );
  buf += hostname_len;
  len += hostname_len;
  
  put_CARD16( &buf, 2, &len );
  memcpy( buf, "Up", 2 );
  buf += 2;
  len += 2;

  if( sendto( sockfd, buffer, len, 0, (struct sockaddr *) &addresse, sizeof( addresse ) ) == -1 )
    printf( "sending failed: %s\n", strerror( errno ) );
}

void do_request( char *message, int length, int sockfd, struct sockaddr_in addresse )
{
  char buffer[1024];
  char *buf = buffer;
  int len = 0;

  printf( "do_request()\n" );

  put_CARD16( &buf, 1, &len );
  put_CARD16( &buf, ACCEPT, &len );
  put_CARD16( &buf, 12, &len );
  put_CARD32( &buf, session_id++, &len );
  put_CARD16( &buf, 0, &len );
  put_CARD16( &buf, 0, &len );
  put_CARD16( &buf, 0, &len );
  put_CARD16( &buf, 0, &len );

  if( sendto( sockfd, buffer, len, 0, (struct sockaddr *) &addresse, sizeof( addresse ) ) == -1 )
    printf( "sending failed: %s\n", strerror( errno ) );
}

void do_keep_alive( char *message, int sockfd, struct sockaddr_in addresse )
{
  int display;
  int session_id;
  char buffer[1024];
  char *buf = buffer;
  int len = 0;

  printf( "do_keep_alive()\n" );
  
  memset( buffer, 0, sizeof( buffer ) );

  display = get_CARD16( &message );
  session_id = get_CARD32( &message );

  put_CARD16( &buf, 1, &len );
  put_CARD16( &buf, ALIVE, &len );
  put_CARD16( &buf, 5, &len );
  put_CARD8( &buf, 1, &len );
  put_CARD32( &buf, session_id, &len );

  sendto( sockfd, buffer, len, 0, (struct sockaddr *) &addresse, sizeof( addresse ) );
}

const char* get_prog_name( const char* path )
{
  const char *buf = path + strlen( path );

  while( buf != path && *buf != '/' )
    buf--;

  return ++buf;
}

void do_manage( char *message, int length, int sockfd, struct sockaddr_in addresse, const char *startup_prog )
{
  int display;
  char buffer[1024];
  int len = 0;

  printf( "do_manage()\n" );

  len = get_CARD32( &message );
  
  if( len == 0 )
    {
      perror( "no display given\n" );
      return;
    }

  display = get_CARD16( &message );

  sprintf( buffer, "%s:%d", inet_ntoa( addresse.sin_addr ), display );

  pid_t pid;

  if( (pid = fork()) == 0 )
    {
      Display *display;

      printf( "DISPLAY is: %s\n", buffer );

      if( (display = XOpenDisplay( buffer ) ) == NULL )
	{	  
	  printf( "cannot open display!\n" );
	  exit(1);
	}

      fcntl( ConnectionNumber( display), F_SETFD, 0 );      
  
      close( sockfd );

      if( setenv( "DISPLAY", buffer, 1 ) == -1 )
	perror( "cannot set display" );
      
      printf( "exec prog command: %s %s\n",  startup_prog, get_prog_name( startup_prog ) );
      
      if( execl( startup_prog, get_prog_name( startup_prog ), NULL ) != 0 )		
		printf( "cannot exec\n" );
      
      printf( "exec done\n" );
      exit(0);
    }
  else
	{
	  child_list.push_back( pid );
	}
}

int check_willing( const char* willing_prog, struct sockaddr_in addresse )
{
  char buffer[1024];

  snprintf( buffer, sizeof( buffer ), "%s %s", willing_prog, inet_ntoa( addresse.sin_addr ) );

  if( system( buffer ) == 0 )
    {
      printf( "request accepted! willing call: %s\n", buffer );
      return 1;
    }

  printf( "request denied! willing call: %s\n", buffer );

  return 0;
}

static void handle_died_child( int signal )
{
  int status = 0;

  pid_t pid = waitpid( -1, &status, WNOHANG );

  if( pid == -1 )
	return; // nothing todo

  printf( "child %d died\n", pid );

  for( std::list<pid_t>::iterator it = child_list.begin(); it != child_list.end(); it++ )
	{
	  if( *it == pid )
		{
		  child_list.erase(it);
		  break;
		}
	}  
}

int main( int argc, char **argv )
{
  int sockfd = 0;

  if( argc != 3 )
    {
      printf( "usage:\n"
	      "%s WILLING_PROG STARTUP_PROG\n",
	      argv[0] );
      printf( "\n"
	      "The WILLING_PROG will be started with: WILLING_PROG IP_ADDRESS\n"
	      "as an accept 0 should be returned and anything else as failure\n"
	      "If the WILLING_PROG returns != 0 no XDMCP-WILLING message will be sent\n"
	      "\n"
	      "The STARTUP_PROG will be started as root\n" );      
      printf( "ldm version: %s (C) 2004 by Martin Oberzalek under the terms of the GPL\n", VERSION );
      return 2;      
    }

  signal( SIGCHLD, handle_died_child );

  sockfd = create_socket();

  if( sockfd <= 0)
    {
      printf( "cannot create UDP socket on Port: %d\n", SERVER_PORT );
      return 1;
    }

  while( 1 )
    {
      char buffer[1024];
      struct sockaddr_in addresse;
      size_t len = sizeof( addresse );
      int version;
      int opcode;
      int length;
      char *buf = buffer;

      memset( buffer, 0, sizeof( buffer ) );

      recvfrom( sockfd, buffer, sizeof( buffer ), 0, (struct sockaddr *) &addresse, &len );
      
      printf( "%s: from: %s:UDP%u : %s \n",
	      argv[0], inet_ntoa( addresse.sin_addr ),
	      ntohs( addresse.sin_port ), buffer );
      
      version = get_CARD16( &buf );
      opcode = get_CARD16( &buf );
      length = get_CARD16( &buf );

      printf( "version: %d opcode: %d length %d\n", version, opcode, length );

      switch( opcode )
	{
	case BROADCAST_QUERY: 
	case QUERY:           
	case INDIRECT_QUERY:  
	case FORWARD_QUERY:   
	  if( check_willing( argv[1], addresse ) )
	    do_query( buf, length, sockfd, addresse ); 
	  break;

	case REQUEST:    do_request( buf, length, sockfd, addresse ); break;
	case MANAGE:     do_manage( buf, length, sockfd, addresse, argv[2] ); break;
	case KEEP_ALIVE: do_keep_alive( buf, sockfd, addresse ); break;
	  
	case WILLING: 
	case UNWILLING:
	case ACCEPT: 
	case DECLINE:
	case FAILED:
	case ALIVE:
	case REFUSE:
	  break;

	default:
	  printf( "unknown opcode: %d\n", opcode );
	}
    }

  close( sockfd );

  return 0;
}
