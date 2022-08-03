#pragma once
//#define AMQPLIBRARY_EXPORTS

#ifdef AMQPLIBRARY_EXPORTS
#define AMQPLIBRARY_API __declspec(dllexport)
#else
#define AMQPLIBRARY_API __declspec(dllimport)
#endif

#include <string>

extern "C" namespace AMQPMessager
{
	extern "C" AMQPLIBRARY_API
	void SendMessageToRabbit(const char *msg, unsigned int len);

	class Storage;
	class Receiver
	{
		Storage* stor;
		std::string Messages;
	public:
		Receiver();
		~Receiver();
		virtual void Destroy();
		virtual void ReceiveMessages();
		virtual std::string GetMessages();
	};

	extern "C" AMQPLIBRARY_API Receiver * create_klass();
}