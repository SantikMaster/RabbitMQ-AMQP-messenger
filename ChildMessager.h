
#ifndef TEST_BASE_H_
#define TEST_BASE_H_

#include "RabbitMQms.h"


class TestAMQP : public RabbitMQms
{
public:
	bool Init(const MqInfo &mqinfo);
	void SetStrPtr(std::string& str);

private:

	virtual void OnRecvedData(const char *data, const uint64_t len);
	std::string* StrPtr;
};

#endif
