
#ifndef TEST_BASE_H_
#define TEST_BASE_H_

#include "RabbitMQms.h"

class TestBase : public RabbitMQms
{
public:
	bool Init(const MqInfo &mqinfo);


private:

	virtual void OnRecvedData(const char *data, const uint64_t len);

};

#endif
