#include <iostream>
#include <string>
#include <fstream>
#include "logger.h"

bool Logger::is_inited = false;
std::string Logger::m_syslogFile = "syslog.txt";
std::string Logger::m_operatorLogFile = "op.txt";


void Logger::init(std::string syslogFileName, std::string operatorLogFileName)
{
	if (is_inited)
	{
		XPRINT("Hey! Logger already inited! Fuck you");
		return;
	}
	is_inited = true;
	m_syslogFile = syslogFileName;
	m_operatorLogFile = operatorLogFileName;
	XPRINT("Logger inited!");
}
