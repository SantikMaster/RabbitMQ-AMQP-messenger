#include "AMQPMessages.h"
#include "ChildMessager.h"
#include <iostream>



void InfoInit(MqInfo& mq);



namespace AMQPMessager
{
	class storage
	{
	public:
		storage() = default;
		MqInfo mq;
		TestBase rmq;
	};
	void SendMessageToRabbit(const char* msg, unsigned int len)
	{
		std::string str = std::string(msg);
		str.resize(len);

		MqInfo mq;
		InfoInit(mq);

		TestBase rmq;
		if (rmq.Init(mq))
		{
			rmq.PublishMsg(str);
		}	
	}
	Receiver::Receiver()
	{
		stor = new storage();

		InfoInit(stor->mq);
	}
	void Receiver::ReceiveMessages()
	{
		if (stor->rmq.Init(stor->mq))
		{
			stor->rmq.StartConsumeMsg("A");
		}	
	}
	Receiver::~Receiver()
	{
		delete stor;
	}
	
}
void InfoInit(MqInfo& mq)
	{
		mq.ip = "127.0.0.1";
		mq.port = 5672;
		mq.loginName = "guest";
		mq.loginPwd = "guest";
		mq.exchangeName = "RmqMgrExchangeTest";
		mq.exchangeType = "fanout";
		mq.queueName = "A"; // queuename
		mq.routingKey = "key 1";
		mq.bindingKey = "";
	}