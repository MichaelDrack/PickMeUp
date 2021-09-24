#pragma once

#include <deque>
#include <memory>
#include <thread>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

// "Encrypt" data


class CConnection;
class CServer;

struct owned_message
{
	std::shared_ptr<CConnection> remote = nullptr;
	std::string msg;

	// Again, a friendly string maker
	friend std::ostream& operator<<(std::ostream& os, const owned_message& msg)
	{
		os << msg.msg;
		return os;
	}
};

class scoped_lock
{
	public:
		scoped_lock(std::mutex& mx): m_mx(mx)
	{
		m_mx.lock();
	}

		~scoped_lock()
		{
			m_mx.unlock();
		}

	private:
		std::mutex& m_mx;
};

template<typename T>
class tsqueue
{
	public:
		tsqueue() = default;
		tsqueue(const tsqueue<T>&) = delete;
		virtual ~tsqueue() { clear(); }

	public:
		// Returns and maintains item at front of Queue
		const T& front()
		{
			scoped_lock lock(muxQueue);
			return deqQueue.front();
		}

		// Returns and maintains item at back of Queue
		const T& back()
		{
			scoped_lock lock(muxQueue);
			return deqQueue.back();
		}

		// Removes and returns item from front of Queue
		T pop_front()
		{
			scoped_lock lock(muxQueue);
			auto t = std::move(deqQueue.front());
			deqQueue.pop_front();
			return t;
		}

		// Removes and returns item from back of Queue
		T pop_back()
		{
			scoped_lock lock(muxQueue);
			auto t = std::move(deqQueue.back());
			deqQueue.pop_back();
			return t;
		}

		// Adds an item to back of Queue
		void push_back(const T& item)
		{
			scoped_lock lock(muxQueue);
			deqQueue.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.notify_one();
		}

		// Adds an item to front of Queue
		void push_front(const T& item)
		{
			scoped_lock lock(muxQueue);
			deqQueue.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.notify_one();
		}

		// Returns true if Queue has no items
		bool empty()
		{
			scoped_lock lock(muxQueue);
			return deqQueue.empty();
		}

		// Returns number of items in Queue
		size_t count()
		{
			scoped_lock lock(muxQueue);
			return deqQueue.size();
		}

		// Clears Queue
		void clear()
		{
			scoped_lock lock(muxQueue);
			deqQueue.clear();
		}

		void wait()
		{
			while (empty())
			{
				std::unique_lock<std::mutex> ul(muxBlocking);
				cvBlocking.wait(ul);
			}
		}

		protected:
			std::mutex muxQueue;
			std::deque<T> deqQueue;
			std::condition_variable cvBlocking;
			std::mutex muxBlocking;
};

class CConnection : public std::enable_shared_from_this<CConnection>
{
	public:
		CConnection(asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message>& qIn):
			m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
		{
			m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

			// Pre-calculate the result for checking when the client responds
			m_nHandshakeCheck = scramble(m_nHandshakeOut);
		}

		void send(const std::string& msg);
		void connectToClient(CServer *server, uint32_t id);

		bool isConnected() { return m_socket.is_open();};
		uint32_t getID() {return id;};
	protected:
		void writeValidation();
		void readValidation(CServer *server);

		size_t readData();
		void addToIncomingMessageQueue();

		void writeData();

		uint64_t scramble(uint64_t nInput)
		{
			uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
			out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
			return out ^ 0xC0DEFACE12345678;
		}

		asio::ip::tcp::socket m_socket;

		// This context is shared with the whole asio instance
		asio::io_context& m_asioContext;

		tsqueue<owned_message>& m_qMessagesIn;
		tsqueue<std::string> m_qMessagesOut;

		std::string m_msgTemporaryIn;

		uint64_t m_nHandshakeOut = 0;
		uint64_t m_nHandshakeIn = 0;
		uint64_t m_nHandshakeCheck = 0;


		bool m_bValidHandshake = false;
		bool m_bConnectionEstablished = false;

		uint32_t id = 0;
};

class CServer
{
	public:
		CServer(uint32_t port): m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{
		}

		bool start();
		void update();

		void messageClient(std::shared_ptr<CConnection> client, const std::string& msg);
	protected:
		virtual bool OnClientConnect(std::shared_ptr<CConnection> client)
		{
			return false;
		}

		// Called when a client appears to have disconnected
		virtual void OnClientDisconnect(std::shared_ptr<CConnection> client)
		{
		}

		// Called when a message arrives
		virtual void OnMessage(std::shared_ptr<CConnection> client, std::string& msg)
		{
		}
	public:
		virtual void OnClientValidated(std::shared_ptr<CConnection> client)
		{
		}
	private:
		void listen_connections();
		bool isConnected();
		tsqueue<owned_message> m_qMessagesIn;

		// Container of active validated connections
		std::deque<std::shared_ptr<CConnection>> m_deqConnections;

		// Order of declaration is important - it is also the order of initialisation
		asio::io_context m_asioContext;
		std::thread m_threadContext;

		// These things need an asio context
		asio::ip::tcp::acceptor m_asioAcceptor;

		uint32_t nClientID = 100;
};
