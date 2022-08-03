#include "AMQPMessages.h"
#include "ChildMessager.h"
#include <iostream>

void InfoInit(MqInfo& mq);

namespace AMQPMessager
{
	class Storage
	{
	public:
		Storage() = default;
		MqInfo mq;
		TestAMQP rmq;
	};
	void SendMessageToRabbit(const char* msg, unsigned int len)
	{
		std::string str = std::string(msg);
		str.resize(len);

		MqInfo mq;
		InfoInit(mq);

		TestAMQP rmq;
		if (rmq.Init(mq))
		{
			rmq.PublishMsg(str);
		}	
	}
	Receiver::Receiver()
	{
		stor = new Storage();

		InfoInit(stor->mq);
		stor->rmq.SetStrPtr(Messages);
		Messages = "";
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
	void Receiver::Destroy()
	{
		delete this;
	}
	std::string Receiver::GetMessages()
	{
		return Messages;
	}

	extern "C" __declspec(dllexport) Receiver * create_klass()
	{
		return new Receiver;
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
