#pragma once
#include <memory>

namespace AMQPMessager
{
	void SendMessageToRabbit(const char *msg, unsigned int len);

	class storage;
	class Receiver
	{
		storage* stor;
	public:
		Receiver();
		~Receiver();
		void ReceiveMessages();
	};
}