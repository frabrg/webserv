#ifndef UTILS_TMP_HPP
# define UTILS_TMP_HPP

# include <string>
# include <sys/time.h>	//get_date
# include <string.h>	//get_date
# include <vector>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include "../srcs/get_next_line/get_next_line.hpp"
# include "Logger.hpp"
// add a std::to_string


class utils_tmp
{
public:
	static std::string	get_date(void);
	static int	isspace(int c);
	static std::vector<std::string> split_string(std::string &str, std::string set);
	static int ft_atoi(const char *str);
	static bool file_exists(const char *filename);
	static std::string read_file(int fd);
};

#endif
