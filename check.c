#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

fd_set afds , wfds , rfds;
char *bufs[FD_SETSIZE];
int ids[FD_SETSIZE] , max_fd , next_id;
char rbuf[1001] , wbuf[64];

void send_all(int form , char *s)
{
    for(int fd = 0 ; fd <= max_fd ; fd++)
        if(FD_ISSET(fd , &wfds) && fd != form)
            send(fd , s , strlen(s) , MSG_NOSIGNAL);
}


int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


int main(int ac , char **av) {
	int sockfd, connfd;
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	if (listen(sockfd, 10) != 0) {
		printf("cannot listen\n"); 
		exit(0); 
	}

    FD_ZERO(&afds);
    FD_SET(sockfd , &afds);
    max_fd = sockfd;

    while(1)
    {
        rfds = wfds = afds;
        if(select(max_fd + 1 , &rfds , &wfds , 0 , 0 ) < 0)
            continue;
        for(int fd = 0 ; fd <= max_fd ; fd++)
        {
            if(!(FD_ISSET(fd , &rfds)))
                continue;
            if(fd == sockfd)
            {
                connfd = accept(sockfd , 0, 0);
                if(connfd < 0)
                    continue;
                if(connfd >= FD_SETSIZE)
                {
                    close(connfd);
                    continue;
                }
                if(connfd > max_fd)
                    max_fd = connfd;
                ids[connfd] = next_id++;
                bufs[connfd] = 0;
                FD_SET(connfd , &afds);
                sprintf(wbuf , "server: client %d just arrived\n" , ids[connfd]);
                send_all(connfd , wbuf);
            }
            else
            {
                int r = recv(fd , rbuf , sizeof(rbuf) -1 , 0);
                if (r <= 0)
                {
                    sprintf(wbuf , "server: client %d just left\n" , ids[fd]);
                    FD_CLR(fd , &afds);
                    close(fd);
                    free(bufs[fd]);
                    bufs[fd] = 0;
                    send_all(fd , wbuf);
                }
                else
                {
                    rbuf[r] = 0;
                    if(!(bufs[fd] = str_join(bufs[fd] , rbuf)))
                        fatal();
                    char *line;
                    int ret;
                    sprintf(wbuf , "client %d: " , ids[fd]);
                    while((ret = extract_message(&bufs[fd] , &line)) == 1)
                    {
                        send_all(fd , wbuf);
                        send_all(fd , line);
                        free(line);
                    }
                    if (ret == -1)
                        fatal();

                }
            }
        }
    }

}