
#include "RabbitMQms.h"


#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <thread>
#include <chrono>


RabbitMQms::RabbitMQms()
{
	m_connection = nullptr;
	m_channelPub = nullptr;
	m_isAutoResume = false;
	m_pTcpMgr = nullptr;
}

RabbitMQms::~RabbitMQms()
{
}

bool RabbitMQms::MqInfoInit(const MqInfo &mqInfo)
{
	try
	{
		m_mqInfo = mqInfo;
	}
	catch (const std::exception &e)
	{
		OnRtnErrMsg("mq info init error" + string(e.what()));
		return false;
	}

	return true;
}

bool RabbitMQms::StartMqInstance()
{
	try
	{
		m_pTcpMgr = TcpMgr::Init(m_mqInfo, this);


		if (!m_pTcpMgr->WaitForReady())
		{
			OnRtnErrMsg(string("start instance error"));
			return false;
		}

		// get the connection
		GetMqConnection();

		// make a channel
		if (!CreateMqChannel())
		{
			return false;
		}
	}
	catch (const std::exception &e)
	{
		OnRtnErrMsg("start mq exception" + string(e.what()));
		return false;
	}

	return true;
}


void RabbitMQms::GetMqConnection()
{
	if (!m_pTcpMgr)
	{
		string err;
		if (!StartMqInstance())
		{
			return;
		}
	}
	else
	{
		m_connection = m_pTcpMgr->GetConnection();
	}
}


bool RabbitMQms::CreateMqChannel()
{
	m_channelPub = boost::make_shared<AMQP::Channel>(m_connection.get());
	m_channelPub->onError([this](const char* message) {
		OnRtnErrMsg("m channel error" + string(message));
		});

	return true;
}


bool RabbitMQms::PublishMsg(const string &msg)
{
	try
	{

		if (m_channelPub != nullptr)
		{
			m_channelPub->publish(m_mqInfo.exchangeName, m_mqInfo.routingKey, msg);
		}
		else std::cout << "m_channelPub is nullptr\n";
	}
	catch (const std::exception &e)
	{
		OnRtnErrMsg(string(e.what()));
		return false;
	}

	return true;
}

bool RabbitMQms::StartConsumeMsg(const string &queueName)
{
	std::cout << "consuming\n";
	try
	{
		m_channelPub->consume(queueName)
			.onReceived(std::bind(&RabbitMQms::ConsumeRecvedCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
			.onError(std::bind(&RabbitMQms::ConsumeErrorCb, this, std::placeholders::_1));
	}
	catch (const std::exception &e)
	{
		OnRtnErrMsg(string(e.what()));
		return false;
	}
	return true;
}
void RabbitMQms::ConsumeErrorCb(const char* msg)
{
	string err;
	err = "Consume error cb" + string(msg);
	OnRtnErrMsg(err);

}
void RabbitMQms::OnRtnErrMsg(const string& err)
{
	std::cerr << "Returned error msg: " << err << "\n";
}

void RabbitMQms::ConsumeRecvedCb(const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
{
	OnRecvedData(message.body(), message.bodySize()); 

	// acknowledge the message
	m_channelPub->ack(deliveryTag);
}

// MyConnectionHandler
MyConnectionHandler::MyConnectionHandler(boost::shared_ptr<TcpMgr> pTcpMgr)
{
	m_pTcpMgr = pTcpMgr;
}

void MyConnectionHandler::onData(AMQP::Connection *connection, const char *data, size_t size)
{
	try
	{
		if (!m_pTcpMgr)
		{
			return;
		}
		string err;
		string msg(data, size);
		m_pTcpMgr->SendData(msg, err);
	}
	catch (const std::exception&e)
	{
		string err = "Send Data error" + string(e.what());
		m_pTcpMgr->OnErrMsg(err );
	}
}

void MyConnectionHandler::onReady(AMQP::Connection *connection)
{
	m_pTcpMgr->SetLoginReady();
}

void MyConnectionHandler::onError(AMQP::Connection *connection, const char *message)
{

	string err = "Error ..." + string(message);
	m_pTcpMgr->OnErrMsg(err);
}

void MyConnectionHandler::onClosed(AMQP::Connection *connection)
{
	string err = "RabbitMq closed";
	m_pTcpMgr->OnErrMsg(err);
}

TcpMgr::TcpMgr()
	: m_work(m_ios),
	m_strand(m_ios),
	m_sock(m_ios),
	m_socketStarted(false),
	m_is_ready(false)
{
}

boost::shared_ptr<TcpMgr> TcpMgr::Init(const MqInfo& mqInfo, RabbitMQms *pRabbitMQms)
{
	boost::shared_ptr<TcpMgr> newTcpMgr(new TcpMgr());
	newTcpMgr->Start(mqInfo, pRabbitMQms);
	return newTcpMgr;
}

void TcpMgr::Run()
{
	m_ios.run();
}

void TcpMgr::Finit()
{
	CloseSocket();
	m_ios.stop();
	m_threads.join_all();
}

void TcpMgr::Start(const MqInfo& mqInfo, RabbitMQms *pRabbitMQms)
{
	m_mqInfo = mqInfo;
	m_pRabbitMQms = pRabbitMQms;
	m_pHandler = new MyConnectionHandler(shared_from_this());
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(m_mqInfo.ip), m_mqInfo.port);
	m_sock.async_connect(ep, m_strand.wrap(boost::bind(&TcpMgr::OnConnect, shared_from_this(), _1)));
	for (auto i = 0; i < kNumOfWorkThreads; ++i)
	{
		m_threads.create_thread(boost::bind(&TcpMgr::Run, this));
	}
}

void TcpMgr::OnConnect(const error_code &err)
{
	if (!err && m_sock.is_open())
	{
		m_socketStarted = true;
		m_pConnect = boost::make_shared<AMQP::Connection>(m_pHandler, AMQP::Login(m_mqInfo.loginName, m_mqInfo.loginPwd));
		RecvData();
	}
	else
	{
		OnErrMsg("connection error" + err.message());
		m_socketStarted = false;
		ReConnectServer();
	}
}

void TcpMgr::RecvData(int32_t total_bytes)
{
	m_strand.post(boost::bind(&TcpMgr::DispatchRecv, shared_from_this(), total_bytes));
}

void TcpMgr::DispatchRecv(int32_t total_bytes)
{
	bool should_start_receive = m_pending_recvs.empty();
	m_pending_recvs.push_back(total_bytes);
	if (should_start_receive)
	{
		StartRecv(total_bytes);
	}
}

void TcpMgr::StartRecv(int32_t total_bytes)
{
	if (total_bytes > 0)
	{
		m_recv_buffer.resize(total_bytes);
		boost::asio::async_read(m_sock, boost::asio::buffer(m_recv_buffer), m_strand.wrap(boost::bind(&TcpMgr::HandleRecv, shared_from_this(), _1, _2)));
	}
	else
	{
		m_recv_buffer.resize(kReceiveBufferSize);
		m_sock.async_read_some(boost::asio::buffer(m_recv_buffer), m_strand.wrap(boost::bind(&TcpMgr::HandleRecv, shared_from_this(), _1, _2)));
	}
}

void TcpMgr::HandleRecv(const error_code &err, size_t bytes)
{
	if (!err)
	{
		m_recv_buffer.resize(bytes);
		{
			m_parseBuf.insert(m_parseBuf.end(), m_recv_buffer.begin(), m_recv_buffer.end());
			ParseAmqpData();
		}
		m_pending_recvs.pop_front();
		if (!m_pending_recvs.empty())
		{
			StartRecv(m_pending_recvs.front());
		}
		else
		{
			RecvData();
		}
	}
	else
	{
		OnErrMsg("handle error" + err.message());
		CloseSocket();
		ReConnectServer();
	}
}

void TcpMgr::ParseAmqpData()
{
	try
	{
		auto data_size = m_parseBuf.size();
		size_t parsed_bytes = 0;
		auto expected_bytes = m_pConnect->expected();
		while (data_size - parsed_bytes >= expected_bytes)
		{
			std::vector<char> buff(m_parseBuf.begin() + parsed_bytes, m_parseBuf.begin() + parsed_bytes + expected_bytes);
			parsed_bytes += m_pConnect->parse(buff.data(), buff.size());
			expected_bytes = m_pConnect->expected();
		}
		m_parseBuf.erase(m_parseBuf.begin(), m_parseBuf.begin() + parsed_bytes);
	}
	catch (const std::exception&e)
	{
		OnErrMsg("Parse amqp data error" + string(e.what()));
	}
}

bool TcpMgr::SendData(const string &msg, std::string errmsg)
{
	try
	{
		auto pmsg(boost::make_shared<string>(msg));
		m_strand.post(boost::bind(&TcpMgr::SendMsg, shared_from_this(), pmsg));
	}
	catch (const std::exception &e)
	{
		errmsg.assign("Send error msg" + string(e.what()));
		return false;
	}

	return true;
}

void TcpMgr::SendMsg(boost::shared_ptr<string> pmsg)
{
	bool should_start_send = m_pending_sends.empty();
	m_pending_sends.emplace_back(pmsg);
	if (should_start_send)
	{
		StartSend();
	}
}

void TcpMgr::StartSend()
{
	if (!m_pending_sends.empty())
	{
		boost::asio::async_write(m_sock, boost::asio::buffer(*m_pending_sends.front().get()), m_strand.wrap(boost::bind(&TcpMgr::HandleWrite, shared_from_this(), _1)));
	}
}

void TcpMgr::HandleWrite(const error_code &err)
{
	if (err)
	{
		OnErrMsg("Handle write" + err.message());
		CloseSocket();
		ReConnectServer();
	}
	else
	{
		m_pending_sends.pop_front();
		StartSend();
	}
}

void TcpMgr::ReConnectServer()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(kTcpRetryInterval));
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(m_mqInfo.ip), m_mqInfo.port);
	m_sock.async_connect(ep, m_strand.wrap(boost::bind(&TcpMgr::OnConnect, shared_from_this(), _1)));
}

void TcpMgr::CloseSocket()
{
	OnErrMsg(string("socket closed"));
	m_strand.post(boost::bind(&TcpMgr::StartCloseSocket, shared_from_this()));
}

void TcpMgr::StartCloseSocket()
{
	if (!m_socketStarted)
	{
		return;
	}
	boost::system::error_code ec;
	m_sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	m_sock.close(ec);
	m_socketStarted = false;
}

void TcpMgr::SetLoginReady()
{
	{
		std::lock_guard<std::mutex> lk(m_mtx_login);
		m_is_ready = true;
	}
	m_cv_login_succ.notify_all();
}

bool TcpMgr::WaitForReady()
{
	std::unique_lock<std::mutex> lk(m_mtx_login);
	return m_cv_login_succ.wait_for(lk, std::chrono::seconds(kLoginRmqTimeOut), [this] {
		return m_is_ready;
	});
}

boost::shared_ptr<AMQP::Connection> TcpMgr::GetConnection()
{
	return m_pConnect;
}

void TcpMgr::OnErrMsg(const std::string& msg)
{
	m_pRabbitMQms->OnRtnErrMsg(msg);
}
