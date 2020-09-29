#include "../includes/Headers.hpp"

/*
** doc:
** https://perso.liris.cnrs.fr/lionel.medini/enseignement/M1IF03/Tutoriels/Tutoriel_CGI_SSI.pdf
*/

/*
** pseudo code create_env_tab:
** 1 créer une map et ajouter toutes les variables nécessaires aux cgi cf. doc
** 2 ajouter les headers de la requêtes avec préfixe 'HTTP_'
** 3 convertir la mpa en un char **env à passer en 3ème argument de execve
*/

// AUTH_TYPE
// CONTENT_LENGTH
// CONTENT_TYPE
// GATEWAY_INTERFACE
// PATH_INFO
// PATH_TRANSLATED
// QUERY_STRING
// REMOTE_ADDR
// REMOTE_IDENT
// REMOTE_USER
// REQUEST_METHOD
// REQUEST_URI
// SCRIPT_NAME

char			**Response::create_env_tab(Request *req)
{
	char											**args_to_tab;
	std::map<std::string, std::string> 				args_to_map;
	std::map<std::string, std::string> 				headers;
	size_t											pos;

	// 1
	args_to_map["GATEWAY_INTERFACE"] = "CGI/1.1"; // version des spécifications CGI utilisées par le serveur
	args_to_map["SERVER_PROTOCOL"] = "HTTP/1.1"; // protocole et version de la requête en cours de traitement
	args_to_map["SERVER_SOFTWARE"] = g_conf._webserv; // nom et version du démon HTTP
	args_to_map["REQUEST_URI"] = req->_uri;
	args_to_map["REQUEST_METHOD"] = req->_method; // méthode associée à la requête en cours de traitement
	args_to_map["REMOTE_ADDR"] = req->_client->_ip; // adresse IP de la machine d'où vient la requête
	args_to_map["PATH_INFO"] = req->_uri; // chaîne entre SCRIPT_PATH et QUERY_STRING dans l'URL
	args_to_map["CONTENT_LENGTH"] = std::to_string(req->_text_body.length()); // longueur des données véhiculées dans la requête (POST)
	if (!req->_query.empty())
		args_to_map["QUERY_STRING"] = req->_query; // données transmises au CGI via l'URL (GET)
	else
		args_to_map["QUERY_STRING"]; // données transmises au CGI via l'URL (GET)
	args_to_map["CONTENT_TYPE"] = req->_content_type; // type MIME des données véhiculées dans la requête
	args_to_map["SCRIPT_NAME"] = req->_location->_cgi_root; // chemin du CGI à partir de la racine du serveur HTTP
	args_to_map["SERVER_NAME"] = req->_client->_server->_name; // nom ou adresse IP de la machine serveur HTTP
	args_to_map["SERVER_PORT"] = std::to_string(req->_client->_server->_port); // numéro du port (TCP) vers lequel la requête a été envoyée
	if (!req->_authorization.empty())
	{
		pos = req->_authorization.find(" ");
		args_to_map["AUTH_TYPE"] = req->_authorization.substr(0, pos); // méthode d'authentification de l'utilisateur s'il y a lieu
		args_to_map["REMOTE_USER"] = req->_authorization.substr(pos + 1); // si authentification, nom de l'utilisateur associé à la requête
		args_to_map["REMOTE_IDENT"] = req->_authorization.substr(pos + 1); // login de connexion de l'utilisateur (pas souvent supporté)
	}
	headers = req->headers_to_map();
	std::map<std::string, std::string>::iterator it = headers.begin();
	while (it != headers.end())
	{
		if (!it->second.empty())
			args_to_map["HTTP_" + it->first] = it->second; // une variable pour chaque champ contenu dans l'en-tête HTTP
		++it;
	}
	args_to_tab = (char **)malloc(sizeof(char *) * (args_to_map.size() + 1));
	it = args_to_map.begin();
	int i = 0;
	while (it != args_to_map.end())
	{
		LOG_WRT(Logger::DEBUG, it->first + " = " +  it->second);
		args_to_tab[i++] = strdup((it->first + "=" + it->second).c_str());
		++it;
	}
	args_to_tab[i] = NULL;
	return (args_to_tab);
}

void		Response::ft_cgi(Request *req)
{
	char 	**env;
    char 	**args;
    int  	tubes[2];
    int		ret;
    int		ret2;
	int		temp_fd;
	pid_t	pid;
	struct stat php;
	std::string	binaire;
	int status;

	if (req->_location->_cgi_root != "")
		binaire = req->_location->_cgi_root;
	else
		binaire = req->_location->_php_root;
    env = create_env_tab(req);
    args = (char **)(malloc(sizeof(char *) * 3));
	if (req->_location->_cgi_root != "")
	    args[0] = strdup(req->_location->_cgi_root.c_str());
	else
      	args[0] = strdup(req->_location->_php_root.c_str());
	args[1] = strdup(req->_file.c_str());
    args[2] = NULL;
	temp_fd = open("./www/temp_file", O_WRONLY | O_CREAT, 0666);
	pipe(tubes);

	if (req->_method == "GET")
		close(tubes[1]);
	if ((pid = fork()) == 0)
	{
		dup2(temp_fd, 1);
		if (stat(binaire.c_str(), &php) != 0 ||
		!(php.st_mode & S_IFREG))
		{
			std::cout << "error CGI\n";
			exit(1);
		}
		dup2(tubes[0], 0);
		errno = 0;
		if ((ret = execve(binaire.c_str(), args, env)) == -1)
		{
			std::cout << "error execve: " << std::string(strerror(errno)) << std::endl;
			exit(1);
		}			
	}
	else
	{
		if (req->_method == "POST")
		{
			ret2 = write(tubes[1], req->_text_body.c_str(), req->_text_body.length());
			if (ret2 == -1)
				_status_code = INTERNAL_ERROR_500;
			else if (ret2 == 0)
				;
			close(tubes[1]);
		}
		waitpid(pid, &status, 0);
		if (WIFEXITED(status))
			LOG_WRT(Logger::DEBUG, "WEXITSTATUS(status) = " + std::to_string(WEXITSTATUS(status)));
		close(tubes[0]);
		close(temp_fd);

		utils_tmp::free_strtab(&args);
		utils_tmp::free_strtab(&env);
    }
}

void        Response::get_cgi_ret(Request *req)
{
    std::ifstream               temp_file("./www/temp_file");
    std::string                 line;
    std::vector<std::string>    split_ret;
    if (temp_file.is_open())
    {
        getline(temp_file, line);
		utils_tmp::remove_return(line);
        LOG_WRT(Logger::DEBUG, "Response::get_cgi_ret(): 1) line = -" + line + "-");
        if (line.find("Status:") != std::string::npos
            && req->_location->_cgi_root != "")
        {
            split_ret = utils_tmp::split(line, ' ');
            _status_code = std::stoi(split_ret[1]);
            split_ret.clear();
        }
        else
            _status_code = OK_200;
        getline(temp_file, line);
		utils_tmp::remove_return(line);
        LOG_WRT(Logger::DEBUG, "Response::get_cgi_ret(): 2) line = -" + line + "-");
        if (line.find("Content-Type:") != std::string::npos
            && req->_location->_cgi_root != "")
        {
            split_ret.clear();
            split_ret = utils_tmp::split(line, ':');
            _content_type[0] = utils_tmp::trim(split_ret[1]);
            split_ret.clear();
        }
        else
            _content_type[0] = "text/html";
    }
}