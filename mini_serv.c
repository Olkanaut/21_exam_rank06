#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>

int id_by_sock[65536];
int max_sock = 0;
int next_id = 0;
fd_set active_socks, rfr, rfw;
char buf_read[42*4096], buf_write[42*4096 + 11];

void	fatal_exit()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	send_msg(int author)
{
	for (int sock = 0; sock <= max_sock; sock++)
	{
		if (FD_ISSET(sock, &rfw) && sock != author)
			send(sock, buf_write, strlen(buf_write), 0);
	}
}

int main(int ac, char **av)
{
	// fd_set active_socks, rfr;
	char  buf_str[42*4096];

	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int port = atoi(av[1]); //(void)port

	bzero(&id_by_sock, sizeof(id_by_sock));
	FD_ZERO(&active_socks);

	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0)
		fatal_exit();

	max_sock = server_sock;
	FD_SET(server_sock, &active_socks);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = (1 << 24) | 127;//htonl(2130706433);
	addr.sin_port = (port >> 8) | (port << 8);//htons(port);
	socklen_t addr_len = sizeof(addr);

	if ((bind(server_sock, (const struct sockaddr*)&addr, sizeof(addr))) < 0)
		fatal_exit();
	if (listen(server_sock, SOMAXCONN) < 0)//if (listen(sock_fd, 0) < 0)
		fatal_exit();

	while (1)
	{
		rfr = rfw = active_socks;
		if (select(max_sock + 1, &rfr, &rfw, NULL, NULL) < 0)
			continue ;

		for (int sock = 0; sock <= max_sock; sock++)
		{
			if (FD_ISSET(sock, &rfr) && sock == server_sock)
			{
				int client_sock = accept(server_sock, (struct sockaddr*)&addr, &addr_len);
				if (client_sock < 0)
					continue ;

				max_sock = (client_sock > max_sock) ? client_sock : max_sock;
				id_by_sock[client_sock] = next_id++;//////
				FD_SET(client_sock, &active_socks);

				sprintf(buf_write, "server: client %d just arrived\n", id_by_sock[client_sock]);
				send_msg(client_sock);
				break ;
			}

			if (FD_ISSET(sock, &rfr) && sock != server_sock)
			{
				int read_bytes = recv(sock, buf_read, 42*4096, 0);
				if (read_bytes <= 0)
				{
					sprintf(buf_write, "server: client %d just left\n", id_by_sock[sock]);
					send_msg(sock);
					FD_CLR(sock, &active_socks);
					close(sock);//
					break ;
				}//else
				for (int i = 0, j = 0; i < read_bytes; i++, j++)
				{
					buf_str[j] = buf_read[i];
					if (buf_str[j] == '\n')
					{
						buf_str[j] = '\0';
						sprintf(buf_write, "client %d: %s\n", id_by_sock[sock], buf_str);
						send_msg(sock);
						j = -1;////
					}
				}
			}
		}
	}
	return (0);
}