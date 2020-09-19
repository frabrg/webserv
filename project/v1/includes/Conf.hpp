#ifndef CONF_HPP
#define CONF_HPP

/*
** Libraries
*/

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/select.h>	// select, fd_set
#include <string.h>		// strerror linux
#include <signal.h>		// signal linux
#include <list>

/*
** Headers
*/

#include "Server.hpp"
#include "Config_parser.hpp"

class Server;

/*
** Class
*/

class Conf
{
    /*
    ** member variables
    */

    private:
        //

    protected:
        //

    public:
        // loop
        bool                    _on;
        std::string             _webserv;

        // select() related
        int                     _nfds;
        fd_set					_readfds;
        fd_set					_save_readfds;
        fd_set					_writefds;
        fd_set					_save_writefds;
        fd_set					_exceptfds;
        fd_set					_save_exceptfds;
        struct timeval			_timeout;
        
        // other variables
        std::vector<Server*>	_servers;
        std::list<int>          _active_fds; // never use ?

    /*
    ** methods
    */

    private:
        void loop_fd_set() const;

    protected:
        //

    public:
        Conf();
		// ~Conf();

        // int parse(char *filename);
        int parse(char *conf);
        void reset_fd_sets(void);
        int get_nfds(void) const;

        void add_fd(int fd);
        void remove_fd(int fd);
        
        int run_select();
};

#endif