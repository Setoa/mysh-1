#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <wait.h>
#include "commands.h"
#include "built_in.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define SOCK_PATH "tpf_unix_sock.server"
#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"
#define DATA "Hello from server"

void server()
{
	int server_sock,client_sock,len,rc;
	int bytes_rec=0;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];
	int backlog=10;
	memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
	memset(&client_sockaddr,0,sizeof(struct sockaddr_un));
	memset(buf,0,256);
	server_sock=socket(AF_UNIX,SOCK_STREAM,0);
	if(server_sock==-1)
	{
		printf("SOCKET ERROR : %d\n",sock_errno());
		exit(1);
	}
	server_sockaddr.sun_family=AF_UNIX;
	strcpy(server_sockaddr.sun_path,SOCK_PATH);
	len=sizeof(server_sockaddr);
	unlink(SOCK_PATH);
	rc=bind(server_sock,(struct sockaddr*)&server_sockaddr, len);
	if(rc==-1)
	{
		printf("BIND ERROR : %d\n",sock_errno());
		close(server_sock);
		exit(1)
	}
	rc=listen(sesrver_sock,backlog);
	if(rc==-1)
		{
			printf("LISTEN ERROR : %d\n",sock_errno());
			close(server_sock);
			exit(1);
		}
	printf("socket listening...\n");
	client_sock=accept(server_sock,(struct sockaddr*)&client_sockaddr, &len);
	if(client_sock==-1)
	{
		printf("ACCEPT ERROR : %d\n",sock_errno());
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	len=sizeof(client_sockaddr);
	rc=getpeername(client_sock,(struct sockaddr*)&client_sockaddr,&len);
	if(rc==-1)
	{
		printf("GETPEERNAME ERROR : %d\n",sock_errno());
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	else printf("DATA RECEIVED = %s\n",buf);
	memset(buf,0,256);
	strcpy(buf,DATA);
	printf("Sending data...\n");
	rc=send(client_sock,buf,strlen(buf),0);
	if(rc==-1)
	{
		printf("SEND ERROR : %d\n",sock_errno());
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	else printf("Data sent!\n");
	close(server_sock);
	close(client_sock);
}

void client()
{
	int client_sock,rc,len;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];
	memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
	memset(&client_sockaddr,0,sizeof(struct sockaddr_un));
	client_sock=socket(AF_UNIX,SOCK_STREAM,0);
	if(client_sock==-1)
	{
		printf("SOCKET ERROR : %d\n",sock_errno());
		exit(1);
	}
	client_sockaddr.sun_family=AF_UNIX;
	strcpy(client_sockaddr.sun_path,CLIENT_PATH);
	len=sizeof(client_sockaddr);
	unlink(CLIENT_PATH);
	rc=bind(client_sock,(struct sockaddr*)&client_sockaddr,len);
	if(rc==-1)
	{
		printf("BIND ERROR : %d\n",sock_errno());
		close(client_sock);
		exit(1);
	}
	server_sockaddr.sun_family=AF_UNIX;
	strcpy(server_sockaddr.sun_path,SERVER_PATH);
	rc=connect(client_sock,(struct sockaddr*)&server_sockaddr,len);
	if(rc==-1)
	{
		printf("CONNECT ERROR = %d\n",sock_errno());
		close(client_sock);
		exit(1);
	}
	strcpy(buf,DATA);
	printf("Sending data...\n");
	rc=send(client_sock,buf,strlen(buf),0);
	if(rc==-1)
	{
		printf("SEND ERROR = %d\n",sock_errno());
		close(client_sock);
		exit(1);
	}
	else printf("Data sent!\n");
	printf("Waiting to receive data...\n");
	memset(buf,0,sizeof(buf));
	rc=recv(client_sock,buf,sizeof(buf));
	if(rc==-1)
	{
		printf("RECV ERROR = %d\n",sock_errno());
		close(client_sock);
		exit(1);
	}
	else printf("DATA RECEIVED = %s\n",buf);
	close(client_sock);
}

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

char** make_back_argv(int argc, char** argv)
{
	char** back_argv=(char**)malloc(sizeof(char*)*(argc-1));
	for(int i=0; i<argc-1; i++)
	{
		back_argv[i]=(char*)malloc(sizeof(char));
		strcpy(back_argv[i],argv[i]);
	}
	return back_argv;
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  int status;
  int pid=0; //process ID
  struct single_command* com;
  if(n_commands ==1) {
    com = (*commands);
		char** back_argv=make_back_argv(com->argc,com->argv);
    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else {
			pid=fork();
			if(pid==0)
			{
				if(strcmp(com->argv[com->argc-1],"&")==0)
				{
					execv(com->argv[0],back_argv);
				}
				else execv(com->argv[0],com->argv);			
      	fprintf(stderr, "%s: command not found\n", com->argv[0]);
      	return -1;
			}
			else if(pid>0)
			{

				if(strcmp(com->argv[com->argc-1],"&")==0) printf("%d\n",pid);
				else
				{
					wait(&status);
					printf("Process Creation Success!\n");
				}
			}
    }
  }
	else if(n_commnads>1)
	{
		//sokcet!

	}
  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
