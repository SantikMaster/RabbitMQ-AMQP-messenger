#include "ChildMessager.h"
#include <iostream>

using namespace std;

int main()
{
	MqInfo mq;
	mq.ip = "127.0.0.1";
	mq.port = 5672;
	mq.loginName = "guest";
	mq.loginPwd = "guest";
	mq.exchangeName = "RmqMgrExchangeTest";
	mq.exchangeType = "fanout"; 
	mq.queueName = "A"; // queuename
	mq.routingKey = "key 1"; 
	string peerQueueName = ""; 
	mq.bindingKey = "";


	TestBase rmq;
	if (rmq.Init(mq))
	{
		rmq.PublishMsg("Unkaine must win"); 
	}
	rmq.StartConsumeMsg("A");
	
	cin.get();
	return 0;
}