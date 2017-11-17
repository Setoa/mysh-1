#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <wait.h>
#include "commands.h"
#include "built_in.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/stat.h>
#define SOCK_PATH "tpf_unix_sock.server"
#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"
#define DATA "Hello from server"

void* server(void* com2)
{
	int server_sock,client_sock,len,rc;
	int bytes_rec=0;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];
	int backlog=10;
	struct single_command* command2=(struct single_command*)com2;
	memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
	memset(&client_sockaddr,0,sizeof(struct sockaddr_un));
	memset(buf,0,256);
	server_sock=socket(AF_UNIX,SOCK_STREAM,0);
	if(server_sock==-1)
	{
		printf("SOCKET ERROR : %d\n",1);
		exit(1);
	}
	server_sockaddr.sun_family=AF_UNIX;
	strcpy(server_sockaddr.sun_path,SOCK_PATH);
	len=sizeof(server_sockaddr);
	unlink(SOCK_PATH);
	rc=bind(server_sock,(struct sockaddr*)&server_sockaddr,len);
	if(rc==-1)
	{
		printf("BIND ERROR : %d\n",2);
		close(server_sock);
		exit(1);
	}
	rc=listen(server_sock,backlog);
	if(rc==-1)
	{
		printf("LISTEN ERROR : %d\n",3);
		close(server_sock);
		exit(1);
	}
	client_sock=accept(server_sock,(struct sockaddr*)&client_sockaddr,&len);
	if(client_sock==-1)
	{
		printf("ACCEPT ERROR : %d\n",4);
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	len=sizeof(client_sockaddr);
	rc=getpeername(client_sock,(struct sockaddr*)&client_sockaddr,&len);
	if(rc==-1)
	{
		printf("GETPEERNAME ERROR : %d\n",5);
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	else printf("%s\n",buf);
	dup2(client_sock,0);
	if(execv(command2->argv[0],command2->argv)==-1)
	{
		fprintf(stderr, "%s: command not found\n", command2->argv[0]);
		exit(1);
	}
	memset(buf,0,256);
	strcpy(buf,DATA);
	
	rc=send(client_sock,buf,strlen(buf),0);
	if(rc==-1)
	{
		printf("SEND ERROR : %d\n",6);
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	close(server_sock);
	close(client_sock);
}

void* client(void* com)
{
	int client_sock,rc,len;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];
	struct single_command* command=(struct single_command*)com;
	memset(&server_sockaddr,0,sizeof(struct sockaddr_un));
	memset(&client_sockaddr,0,sizeof(struct sockaddr_un));
	client_sock=socket(AF_UNIX,SOCK_STREAM,0);
	if(client_sock==-1)
	{
		printf("SOCKET ERROR : %d\n",11);
		exit(1);
	}
	client_sockaddr.sun_family=AF_UNIX;
	strcpy(client_sockaddr.sun_path,CLIENT_PATH);
	len=sizeof(client_sockaddr);
	unlink(CLIENT_PATH);
	rc=bind(client_sock,(struct sockaddr*)&client_sockaddr,len);
	if(rc==-1)
	{
		printf("BIND ERROR : %d\n",12);
		close(client_sock);
		exit(1);
	}
	server_sockaddr.sun_family=AF_UNIX;
	strcpy(server_sockaddr.sun_path,SERVER_PATH);
	rc=connect(client_sock,(struct sockaddr*)&server_sockaddr,len);
	if(rc==-1)
	{
		printf("CONNECT ERROR : %d\n",13);
		close(client_sock);
		exit(1);
	}
	dup2(client_sock,1);
	if(execv(command->argv[0],command->argv)==-1)
	{
		fprintf(stderr, "%s: command not found\n", command->argv[0]);
		exit(1);
	}
	rc=send(client_sock,buf,strlen(buf),0);
	if(rc==-1)
	{
		printf("SEND ERROR : %d\n",14);
		close(client_sock);
		exit(1);
	}
	memset(buf,0,sizeof(buf));
	rc=recv(client_sock,buf,sizeof(buf),0);
	if(rc==-1)
	{
		printf("RECV ERROR : %d\n",15);
		close(client_sock);
		exit(1);
	}
	else printf("%s\n",buf);
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

char** make_back_com(int argc, char** argv)
{
	char** back_com=(char**)malloc(sizeof(char*)*(argc-1));
	for(int i=0; i<argc-1; i++)
	{
		back_com[i]=(char*)malloc(sizeof(char));
		strcpy(back_com[i],argv[i]);
	}
	return back_com;
}

char* path_resolution(char* com)
{
	char* resol=(char*)malloc(sizeof(char)*256); //resolution
	char* token;
	char* env=getenv("PATH");
	struct stat condition;
	if(com==NULL) return NULL;
	token=strtok(env,":");
	while(token!=NULL)
	{
	  memset(resol,0,256);
	  strcat(resol,token);
	  strcat(resol,"/");
	  strcat(resol,com);
		memset(&condition,0,sizeof(stat));
	  if(S_ISREG(condition.st_mode) && stat(resol,&condition)!=-1) break;
	  token=strtok(NULL,":");
	}
	return resol;
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
	pthread_t thread_c;
	pthread_t thread_s;
	int p_status1; //multi commands
	int p_status2;
	int status; //single commands
	char* resolution;
	int isbackargv=0;
	int t_status1;
	int t_status2;
	int pid1; //process ID(multi commands)
	int pid2;
	int sinpid; //process ID(single command)
	struct single_command* com =(*commands);

	if(n_commands > 0) {
		
    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
					return -1;
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
			if(com->argv[0][0]!='/')
			{
			  resolution=path_resolution(com->argv[0]); 
			  strcpy(com->argv[0],resolution);
			  com->argc--;
			}

			if(n_commands>1)
			{
				pid1=fork();
				char* resolution2;
				struct single_command* com2=*commands+1;
				if(com2->argv[0][0]!='/')
				{
					resolution2=path_resolution(com2->argv[0]);
				  strcpy(com2->argv[0],resolution2);
					com2->argc--;
				}
				if(pid1>0) wait(&p_status1);
				else if(pid1==0)
				{
					pid2=fork();
					if(pid2>0)
					{	
						waitpid(-1,&p_status2,WNOHANG);
						if(pthread_create(&thread_s,NULL,server,com2)<0) printf("thread error!\n");
						pthread_join(thread_s,(void**)&t_status1);
						wait(&p_status2);
					}
					else if(pid2==0)
					{
						if(pthread_create(&thread_c,NULL,client,(void*)com)<0) printf("thread error!\n");
						pthread_join(thread_c,(void**)&t_status2);
					}
					else printf("Fork Error!\n");
				}
				else printf("Fork Error!\n");
			}
			else //n_commands==1
			{
				sinpid=fork();
				if(sinpid==0)
				{
					if(strcmp(com->argv[com->argc-1],"&")==0)
					{
						back.back_argv=make_back_com(com->argc,com->argv);
						isbackargv=1;
						if(execv(back.back_argv[0],back.back_argv)==-1)
						{
							fprintf(stderr,"%s: command not found\n",back.back_argv[0]);
							return -1;
						}
						back.back_pid=getpid();
						back.back_argc=com->argc;
					}
					else
					{
						if(execv(com->argv[0],com->argv)==-1)
						{
							fprintf(stderr, "%s: command not found\n", com->argv[0]);
							return -1;
						}
					}
					if(isbackargv) printf("%d\n",back.back_pid); //process id
				}
				else if(sinpid>0)
				{
					if(!isbackargv)
					{
						wait(&status);
						
					}
				}
				else printf("FORK ERROR!\n");
    	}
		}
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
