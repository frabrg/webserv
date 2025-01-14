#include "../includes/Headers.hpp"

Conf g_conf;

void print_clients_of_all_servers(void)
{
    Server *s;
    Client *c;
    LOG_WRT(Logger::DEBUG, "-------------------------------");
    std::vector<Server*>::iterator it_s = g_conf._servers.begin();
    for (; it_s != g_conf._servers.end(); it_s++)
    {
        s = *it_s;
        LOG_WRT(Logger::DEBUG, "SERVER : " + s->_name);
        std::vector<Client*>::iterator it_c = s->_clients.begin();
        for (; it_c != s->_clients.end(); it_c++)
        {
            c = *it_c;
            LOG_WRT(Logger::DEBUG, "<==>CLIENT fd=" + std::to_string(c->_accept_fd));
        }
    }
    LOG_WRT(Logger::DEBUG, "-------------------------------");
}

void print_all_fd(void)
{
	std::list<int>::iterator it_fd = g_conf._active_fds.begin();
	std::string fds;

	LOG_WRT(Logger::DEBUG, "LIST _active_fds (size=" + std::to_string(g_conf._active_fds.size()) + ")");
	//g_conf._active_fds.sort();
	for (; it_fd != g_conf._active_fds.end(); ++it_fd)
	{
		fds += std::to_string(*it_fd) + " | ";
	}
	fds += "\n";
	LOG_WRT(Logger::DEBUG, fds);
	LOG_WRT(Logger::DEBUG, "-------------------------------");
}

void print_save_fds(void)
{
	std::list<int>::iterator it_fd;
	int nb;

	it_fd = g_conf._active_fds.begin();
	std::string saved_read_fds;
	nb = 0;

	for (; it_fd != g_conf._active_fds.end(); ++it_fd)
	{
		if (FD_ISSET(*it_fd, &g_conf._save_readfds))
		{
			saved_read_fds += std::to_string(*it_fd) + ", ";
			nb++;
		}
	}
	saved_read_fds = "saved_read_fds (nb=" + std::to_string(nb) + "): " + saved_read_fds;
	LOG_WRT(Logger::DEBUG, std::string(CYAN_C) + saved_read_fds + std::string(RESET));
	
	it_fd = g_conf._active_fds.begin();
	std::string saved_write_fds;
	nb = 0;

	for (; it_fd != g_conf._active_fds.end(); ++it_fd)
	{
		if (FD_ISSET(*it_fd, &g_conf._save_writefds))
		{
			saved_write_fds += std::to_string(*it_fd) + ", ";
			nb++;
		}
	}
	saved_write_fds = "saved_write_fds (nb=" + std::to_string(nb) + "): " + saved_write_fds;
	LOG_WRT(Logger::DEBUG, std::string(CYAN_C) + saved_write_fds + std::string(RESET));

	LOG_WRT(Logger::DEBUG, "-------------------------------");
}

void print_set_fds(void)
{
	std::list<int>::iterator it_fd;
	int nb;

	it_fd = g_conf._active_fds.begin();
	std::string set_read_fds;
	nb = 0;

	for (; it_fd != g_conf._active_fds.end(); ++it_fd)
	{
		if (FD_ISSET(*it_fd, &g_conf._readfds))
		{
			set_read_fds += std::to_string(*it_fd) + ", ";
			nb++;
		}
	}
	set_read_fds = "set_read_fds (nb=" + std::to_string(nb) + "): " + set_read_fds;
	LOG_WRT(Logger::DEBUG, std::string(MAGENTA_C) + set_read_fds + std::string(RESET));
	
	it_fd = g_conf._active_fds.begin();
	std::string set_write_fds;
	nb = 0;

	for (; it_fd != g_conf._active_fds.end(); ++it_fd)
	{
		if (FD_ISSET(*it_fd, &g_conf._writefds))
		{
			set_write_fds += std::to_string(*it_fd) + ", ";
			nb++;
		}
	}
	set_write_fds = "set_write_fds (nb=" + std::to_string(nb) + "): " + set_write_fds;
	LOG_WRT(Logger::DEBUG,  std::string(MAGENTA_C) + set_write_fds + std::string(RESET));

	LOG_WRT(Logger::DEBUG, "-------------------------------");
}

int erase_client_from_vector(Server *s, std::vector<Client*> &v, std::vector<Client*>::iterator &it_c)
{
	Client *c = *it_c;
	delete c;
	it_c = v.erase(it_c);
	LOG_WRT(Logger::INFO, s->_name + " has now " + std::to_string(v.size()) + " client(s) connected");
	if (v.empty())
		return (1);
	else
		it_c = v.begin();
	return (0);
}


void	shutdown(int sig)
{
	Server *s;
	std::vector<Server*>::iterator it_s;
	Client *c;
	std::vector<Client*>::iterator it_c;
	Location *l;
	std::vector<Location*>::iterator it_l;
	
	g_conf._on = false;
	LOG_WRT(Logger::INFO, "\33[2K\r" + g_conf._webserv + " deleting clients ...");

	for (it_s = g_conf._servers.begin(); it_s != g_conf._servers.end(); ++it_s)
	{
		s = *it_s;
		for (it_c = s->_clients.begin(); it_c!= s->_clients.end(); ++it_c)
			delete *it_c;
		for (it_c = s->_clients_503.begin(); it_c!= s->_clients_503.end(); ++it_c)
			delete *it_c;
		for (it_l = s->_locations.begin(); it_l!= s->_locations.end(); ++it_l)
			delete *it_l;
	}
	for (it_s = g_conf._servers.begin(); it_s != g_conf._servers.end(); ++it_s)
		delete *it_s;
	g_conf._servers.clear();

	g_conf._servers.clear();
	
	LOG_WRT(Logger::INFO, "\33[2K\r" + g_conf._webserv + " server size " + std::to_string(g_conf._servers.size()));
	print_clients_of_all_servers();
	LOG_WRT(Logger::INFO, "\33[2K\r" + g_conf._webserv + " status off");
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	Server *s;
	Client *c;
	std::vector<Client*>::iterator it_c;
	int select_ret = 0;

	LOG_START(Logger::DEBUG, "", false);
	signal(SIGINT, shutdown);
	if (argc != 2 || !g_conf.parse(argv[1]))
        return (EXIT_ERROR);
    while (g_conf._on)
    {
		print_save_fds();
		select_ret = g_conf.run_select();
		print_set_fds();
		if (select_ret == -1)
		{
			LOG_WRT(Logger::INFO, std::string(RED_C) + "break on select() == -1" + std::string(RESET));
			break ;
		}
		else if (select_ret == 0)
		{
			LOG_WRT(Logger::INFO, std::string(RED_C) + "break on select() == 0" + std::string(RESET));
			break ;
		}		
		else
		{
			print_all_fd();
			std::vector<Server*>::iterator it_s = g_conf._servers.begin();
			for (; it_s != g_conf._servers.end(); it_s++)
			{
				s = *it_s;
				LOG_WRT(Logger::DEBUG, "iterating over server:" + s->_name);
				// pour chaque serveur:
				// 1 - on accepte, s'il y en a une, la demande de connexion du client auprès du serveur it_s (FD_ISSET())
				// 2 - on itère sur les clients_503 du serveur pour les servir
				// 3 - on itère sur les clients du serveur pour les servir

				// FD_ISSET(): check si le fd est dans le set (de ceux qui sont prêts à être lu, grâce au select).
				// the socket is "readable" if the remote peer (the client) closes it / select() returns if a read() will not block, not only when there's data available (meaning also if EOF)
				// (https://stackoverflow.com/questions/6453149/select-says-socket-is-ready-to-read-when-it-is-definitely-not-actually-is-close)
				if (FD_ISSET(s->_socket_fd, &g_conf._readfds))
				{
					LOG_WRT(Logger::INFO, std::string(GREEN_C) + "new client on server " + s->_name + std::string(RESET));
					s->acceptNewClient();
					g_conf._nb_accepted_connections += 1;
					LOG_WRT(Logger::INFO, std::string(YELLOW_C) + "_nb_accepted_connections = " + std::to_string(g_conf._nb_accepted_connections) + std::string(RESET));
				}
		
				for (it_c = s->_clients_503.begin(); it_c != s->_clients_503.end(); it_c++)
				{
					LOG_WRT(Logger::DEBUG, "iterating over client 503:" + std::to_string(c->_accept_fd));
					exit(EXIT_FAILURE);
					c = *it_c;
					if (!c->_is_connected)
					{
						if (erase_client_from_vector(s, s->_clients_503, it_c))
							break;
						else
							continue ;
					}
					if (!c->_is_finished)
					{
						if (c->_wfd != -1 && c->_read_ok == 1)
						{
							if (FD_ISSET(c->_wfd, &g_conf._writefds))
								c->write_file(); 
						}
						if (c->_rfd != -1)
						{
							if (FD_ISSET(c->_rfd, &g_conf._readfds))
								c->read_file(c->_response._body); 
						}
						if (c->_read_ok == 1)	
							s->handleClientRequest(c);
					}
					if (c->_is_finished)
							c->reset();
					if (utils_tmp::getSecondsDiff(c->_last_active_time) >= CLIENT_CONNECTION_TIMEOUT)
					{
						LOG_WRT(Logger::DEBUG, "client "
											+ std::to_string(c->_accept_fd)
											+ " TIMEOUT "
											+ std::to_string(CLIENT_CONNECTION_TIMEOUT));
						if (erase_client_from_vector(s, s->_clients_503, it_c))
							break;
						else
							continue ;
					}
				}

				for (it_c = s->_clients.begin(); it_c != s->_clients.end(); it_c++)
				{
					c = *it_c;
					LOG_WRT(Logger::DEBUG, "iterating over existing client:" + std::to_string(c->_accept_fd));
					if (!c->_is_connected)
					{
						if (erase_client_from_vector(s, s->_clients, it_c))
							break;
						else
							continue ;
					}
					if (!c->_is_finished)
					{
						if (c->_wfd != -1 && c->_read_ok == 1)
						{
							if (FD_ISSET(c->_wfd, &g_conf._writefds))
								c->write_file();
						}
						if (c->_rfd != -1)
						{ 
							if (FD_ISSET(c->_rfd, &g_conf._readfds))
								c->read_file(c->_response._body); 
						}
						if (c->_read_ok == 1)
							s->handleClientRequest(c);
					}
					
					LOG_WRT(Logger::DEBUG, "client " + std::to_string(c->_accept_fd)
										+ " secondsDiff = " + std::to_string(utils_tmp::getSecondsDiff(c->_last_active_time)));
					
					if (c->_is_finished)
						c->reset();

					if (utils_tmp::getSecondsDiff(c->_last_active_time) >= CLIENT_CONNECTION_TIMEOUT)
					{
						LOG_WRT(Logger::DEBUG, "client "
											+ std::to_string(c->_accept_fd)
											+ " TIMEOUT "
											+ std::to_string(CLIENT_CONNECTION_TIMEOUT));
						if (erase_client_from_vector(s, s->_clients, it_c))
							break;
						else
							continue ;
					}
				}
			}
			LOG_WRT(Logger::DEBUG, "---------------\n\n");
		}
    }
	shutdown(1);
    return (EXIT_SUCCESS);
}
