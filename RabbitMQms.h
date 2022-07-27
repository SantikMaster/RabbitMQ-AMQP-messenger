#ifndef TG_MGR_BASE_H_
#define TG_MGR_BASE_H_
#include <string>
#include <amqpcpp.h>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

using std::string;

typedef struct _mqinfo
{
	string ip;
	unsigned short port;
	string loginName;
	string loginPwd;
	string exchangeName;
	string exchangeType;
	string queueName;
	string routingKey; 
	string bindingKey; 
} MqInfo;

class MyConnectionHandler;
class TcpMgr;

class RabbitMQms
{
public:
	RabbitMQms();
	virtual ~RabbitMQms();
	bool StartConsumeMsg(const string& queueName);

	bool MqInfoInit(const MqInfo &mqInfo);

	bool StartMqInstance();

	bool PublishMsg(const string &msg);



	void OnRtnErrMsg(const string &err);

protected:

	virtual void OnRecvedData(const char *data, const uint64_t len) = 0;


private:
	MqInfo m_mqInfo;
	boost::shared_ptr<AMQP::Connection > m_connection;
	boost::shared_ptr<AMQP::Channel> m_channelPub;
	bool m_isAutoResume;
	boost::shared_ptr<TcpMgr> m_pTcpMgr;

	void GetMqConnection();
	bool CreateMqChannel();
	void ConsumeRecvedCb(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered);
	void ConsumeErrorCb(const char *msg);
};


class MyConnectionHandler : public AMQP::ConnectionHandler
{
private:
	boost::shared_ptr<TcpMgr> m_pTcpMgr;

public:
	MyConnectionHandler(boost::shared_ptr<TcpMgr> pTcpMgr);


	virtual void onData(AMQP::Connection *connection, const char *data, size_t size);
	virtual void onReady(AMQP::Connection *connection);
	virtual void onError(AMQP::Connection *connection, const char *message);
	virtual void onClosed(AMQP::Connection *connection);
};

class TcpMgr : public boost::enable_shared_from_this<TcpMgr>, boost::noncopyable
{
public:
	static boost::shared_ptr<TcpMgr> Init(const MqInfo& mqInfo, RabbitMQms *pRabbitMQms);
	boost::shared_ptr<AMQP::Connection> GetConnection();
	bool SendData(const std::string &msg, std::string errmsg);
	bool WaitForReady();
	void SetLoginReady();
	void Finit();
	void OnErrMsg(std::string& msg);

private:
	using error_code = boost::system::error_code;
	MqInfo m_mqInfo;
	boost::asio::io_service m_ios;
	boost::asio::io_service::work m_work;
	boost::asio::io_service::strand m_strand;
	boost::asio::ip::tcp::socket m_sock;
	static const int kNumOfWorkThreads = 1; 
	boost::thread_group m_threads;
	bool m_socketStarted;
	std::vector<char> m_recv_buffer;
	std::vector<char> m_parseBuf; 
	std::list<int32_t> m_pending_recvs;
	static const int kReceiveBufferSize = 4096; 
	std::list<boost::shared_ptr<std::string>> m_pending_sends;
	RabbitMQms *m_pRabbitMQms;
	MyConnectionHandler* m_pHandler;
	boost::shared_ptr<AMQP::Connection> m_pConnect;
	static const int kTcpRetryInterval = 1000; //ms
	static const int kLoginRmqTimeOut = 10; //s
	std::condition_variable m_cv_login_succ;
	bool m_is_ready;
	std::mutex m_mtx_login;

	TcpMgr();
	void Start(const MqInfo& mqInfo, RabbitMQms *pRabbitMQms);
	void OnConnect(const error_code &err);
	void HandleWrite(const error_code &err);
	void ReConnectServer();
	void RecvData(int32_t total_bytes = 0);
	void DispatchRecv(int32_t total_bytes);
	void StartRecv(int32_t total_bytes);
	void HandleRecv(const error_code &err, size_t bytes);
	void SendMsg(boost::shared_ptr<string> msg);
	void StartSend();
	void Run();
	void CloseSocket();
	void StartCloseSocket();
	void ParseAmqpData();
};

#endif
