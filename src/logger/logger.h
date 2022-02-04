#ifndef __LOGGER__
#define __LOGGER__

#include <string>
#include <string.h>
#include <fstream>

#define LOG_HEADER "[LOGGER]"
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define XINFO(msg, ...) Logger::log(LOG_HEADER, msg, #__VA_ARGS__)
#define XPRINT(msg, ...) Logger::print(LOG_HEADER, msg, #__VA_ARGS__)

class Logger
{
public:
	template <typename T, typename... Types>
	static void log(T var1, Types... var2)
	{
		std::ofstream os(m_syslogFile, std::ofstream::app);
		if (!os.is_open())
		{
			print("Fuck! Unable to open syslog file");
			return;
		}
		os << __TIME__ << " " << __FILENAME__ << ":" << __LINE__ << " "  << std::forward<T>(var1);
		using expander = int[];
		(void)expander{0, (void(os << ' ' << std::forward<Types>(var2)), 0)...};

		os << std::endl;
	}

	//TODO
	template <typename T, typename... Types>
	static void operalogLog(T var1, Types... var2);

	// Variadic function Template that takes
	// variable number of arguments and prints
	// all of them.
	template <typename T, typename... Types>
	static void print(T var1, Types... var2)
	{
		std::cout << __TIME__ << " " << __FILENAME__ << ":" << __LINE__ << " "  << std::forward<T>(var1);

		using expander = int[];
		(void)expander{0, (void(std::cout << std::forward<Types>(var2)), 0)...};

		std::cout << std::endl;
	}

	static void init(std::string syslogFileName, std::string operatorLogFileName);

private:
	Logger() {}

	static std::string m_syslogFile;
	static std::string m_operatorLogFile;

	static bool is_inited;
};

#endif
