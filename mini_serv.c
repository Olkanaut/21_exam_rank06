#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>

int ids[65536];
int max_sock = 0;
int serverFD;
int idCount = 0;
fd_set	fds_active, fds_read, fds_write;
char	buf_read[42*4096], buf_write[42*4096 + 11];
struct sockaddr_in addr;

void	fatal_exit()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	send_msg(int author)
{
	for (int sock = 0; sock <= max_sock; sock++)
	{
		if (FD_ISSET(sock, &fds_write) && sock != author)
			send(sock, buf_write, strlen(buf_write), 0);
	}
}

int accept_client()
{
	socklen_t addr_len = sizeof(addr);
	int clientFd = accept(serverFD, (struct sockaddr*)&addr, &addr_len);
	if (clientFd < 0)
		return 1;

	max_sock = (clientFd > max_sock) ? clientFd : max_sock;
	ids[clientFd] = idCount++;
	FD_SET(clientFd, &fds_active);

	sprintf(buf_write, "server: client %d just arrived\n", ids[clientFd]);
	send_msg(clientFd);
	return 0;
}

int main(int ac, char **av)
{
	char  buf_str[42*4096];

	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int port = atoi(av[1]); //(void)port

	bzero(&ids, sizeof(ids));
	FD_ZERO(&fds_active);

	serverFD = socket(AF_INET, SOCK_STREAM, 0);//was local
	if (serverFD < 0)
		fatal_exit();

	max_sock = serverFD;
	FD_SET(serverFD, &fds_active);

	// struct sockaddr_in addr;//memset(&_sockAddr, 0, sizeof(_sockAddr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = (1 << 24) | 127;//htonl(2130706433);
	addr.sin_port = htons(port);
	socklen_t addr_len = sizeof(addr);

	if (bind(serverFD, (const struct sockaddr*)&addr, addr_len))
		fatal_exit();
	if (listen(serverFD, SOMAXCONN))//SOMAXCONN or 0//150
		fatal_exit();

	while (1)
	{
		fds_read = fds_write = fds_active;
		if (select(max_sock + 1, &fds_read, &fds_write, NULL, NULL) < 0)
			continue ;

		for (int sock = 0; sock <= max_sock; sock++)
		{
			if (FD_ISSET(sock, &fds_read) == 0)
				continue ;

			if (sock == serverFD)
			{
				if (accept_client())
					continue ;
				break ;
			}

			int read_bytes = recv(sock, buf_read, 42*4096, 0);
			if (read_bytes <= 0)
			{
				sprintf(buf_write, "server: client %d just left\n", ids[sock]);
				send_msg(sock);
				FD_CLR(sock, &fds_active);
				close(sock);//
				break ;
			}
			for (int i = 0, j = 0; i < read_bytes; i++, j++)
			{
				buf_str[j] = buf_read[i];
				if (buf_str[j] == '\n')
				{
					buf_str[j] = '\0';
					sprintf(buf_write, "client %d: %s\n", ids[sock], buf_str);
					send_msg(sock);
					j = -1;////
				}
			}
		}
	}
	return (0);
}