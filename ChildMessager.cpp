#include "ChildMessager.h"
#include <iostream>


bool TestBase::Init(const MqInfo &mqinfo)
{
	string err;
	if (!MqInfoInit(mqinfo))
	{
		std::cerr << "mq info init error new: \n";
		return false;
	}
	if (!StartMqInstance())
	{
		std::cerr << "StartMqInstance error: \n";
		return false;
	}

	return true;
}

void TestBase::OnRecvedData(const char *data, const uint64_t len)
{
	std::string str = std::string(data);
	str.resize(len);
	std::cout << str << "!  \n";
}
