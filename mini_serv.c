#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>

int	count = 0, max_fd = 0;
int	ids[65536];
fd_set	read_fd, write_fd, active_fd;
char	buf_read[4096*42], buf_write[4096*42 + 11];

void	error_exit()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	notify_other(int author)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &write_fd) && fd != author)//
			send(fd, &buf_write, strlen(buf_write), 0);
	}
}

int		accept_client(int serverFD, struct sockaddr_in *addr)
{
	socklen_t len = sizeof(addr);
	int cliendFd = accept(serverFD, (struct sockaddr *)&addr, &len);
	if (cliendFd < 0)
		return 1;

	max_fd = cliendFd > max_fd ? cliendFd : max_fd;
	ids[cliendFd] = count++;

	FD_SET(cliendFd, &active_fd);
	sprintf(buf_write, "server: client %d just arrived\n", ids[cliendFd]);
	notify_other(cliendFd);
	return 0;
}

int		recieve_msg_and_detach_client(int fd)
{
	int read_bytes = recv(fd, &buf_read, 4096*42, 0);
	if (read_bytes <= 0)
	{
		sprintf(buf_write, "server: client %d just left\n", ids[fd]);
		notify_other(fd);
		FD_CLR(fd, &active_fd);
		close(fd);
	}
	return read_bytes;
}

void	send_msg(int fd, int read_bytes)
{
	char buf_str[4096*42];

	for (int i = 0, j = 0; i < read_bytes; i++, j++)
	{
		buf_str[j] = buf_read[i];//
		if (buf_str[j] == '\n')
		{
			buf_str[j] = '\0';
			sprintf(buf_write, "client %d: %s\n", ids[fd], buf_str);
			notify_other(fd);
			j = -1;
		}
	}
}

int		main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int port = atoi(av[1]);
	bzero(&ids, sizeof(ids));
	FD_ZERO(&active_fd);

	int serverFD = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFD < 0)
		error_exit();

	max_fd = serverFD;
	FD_SET(serverFD, &active_fd);//

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = (1 << 24) | 127;
	addr.sin_port = htons(port);

	if (bind(serverFD, (const struct sockaddr *)&addr, sizeof(addr)))
		error_exit();
	if (listen(serverFD, SOMAXCONN))
		error_exit();

	while (1)
	{
		read_fd = write_fd = active_fd;
		if (select(max_fd + 1, &read_fd, &write_fd, NULL, NULL) < 0)
			continue ;

		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &read_fd) == 0)
				continue ;

			if (fd == serverFD)
			{
				if (accept_client(serverFD, &addr))
					continue ;
				break ;
			}

			int read_bytes = recieve_msg_and_detach_client(fd);
			if (read_bytes <= 0)
				break ;
			send_msg(fd, read_bytes);
		}
	}
	return 0;
}
