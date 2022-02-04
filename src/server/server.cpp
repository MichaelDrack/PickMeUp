#include <iostream>
#include "server.h"
#include "logger/logger.h"

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#define LOG_HEADER "[SERVER]"

void CServer::update()
{
	m_qMessagesIn.wait();

	// Process as many messages as you can up to the value
	// specified
	while (!m_qMessagesIn.empty())
	{
		// Grab the front message
		owned_message msg = m_qMessagesIn.pop_front();

		// Pass to message handler
		OnMessage(msg.remote, msg.msg);
	}
}

void CServer::listen_connections()
{
	m_asioAcceptor.async_accept(
		[this](std::error_code ec, asio::ip::tcp::socket socket)
		{
			// Triggered by incoming connection request
			if (!ec)
			{
				// Display some useful(?) information
				XPRINT("New Connection: ", socket.remote_endpoint());

				//Create a new connection to handle this client
				std::shared_ptr<CConnection> newconn = std::make_shared<CConnection>(m_asioContext, std::move(socket), m_qMessagesIn);

				m_deqConnections.push_back(std::move(newconn));

				m_deqConnections.back()->connectToClient(this, nClientID++);
			}
			else
			{
				// Error has occurred during acceptance
				XPRINT("New Connection Error: ", ec.message());
			}

			this->listen_connections();
		});
}

void CServer::messageClient(std::shared_ptr<CConnection> client, const std::string& msg)
{
	if (client && client->isConnected())
	{
		// ...and post the message via the connection
		client->send(msg);
	}
	else
	{
		// If we cant communicate with client then we may as 
		// well remove the client - let the server know, it may
		// be tracking it somehow
		OnClientDisconnect(client);

		// Off you go now, bye bye!
		client.reset();

		// Then physically remove it from the container
		m_deqConnections.erase( std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
	}
}

bool CServer::start()
{
	try
	{
		listen_connections();

		m_threadContext = std::thread([this]() { m_asioContext.run(); });
	}
	catch (std::exception& e)
	{
		// Something prohibited the server from listening
		std::cerr << "[SERVER] Exception: " << e.what() << "\n";
		return false;
	}

	XPRINT("Started!");
	return true;
}

void CConnection::writeValidation()
{
	asio::async_write(m_socket, asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
		[this](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				XPRINT("[", id, "] Validation sent, let`s wait for a response");
				// Validation data sent, clients should sit and wait
				// for a response (or a closure)
			}
			else
			{
				m_socket.close();
			}
		});
}

void CConnection::readValidation(CServer *server)
{
	asio::async_read(m_socket, asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
		[this, server](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				// Compare sent data to actual solution
				//if (m_nHandshakeIn == m_nHandshakeCheck)
				{
					// Client has provided valid solution, so allow it to connect properly
					XPRINT("Client Validated");
					server->OnClientValidated(this->shared_from_this());

					// Sit waiting to receive data now
					readData();
				}
			//	else
			//	{
					// Client gave incorrect data, so disconnect
			//		XPRINT("Client Disconnected (Fail Validation)");
			//		m_socket.close();
			//	}
			} else
			{
				// Some biggerfailure occured
				XPRINT("Client Disconnected (ReadValidation)");
				m_socket.close();
			}
		});
}

void CConnection::connectToClient(CServer *server, uint32_t uid)
{
	if (!m_socket.is_open())
	{
		XPRINT("ERROR: socket is not open, restart server");
		return;
	}

	id = uid;

	writeValidation();

	readValidation(server);
}

void CConnection::addToIncomingMessageQueue()
{
	m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });

	readData();
}

void CConnection::writeData()
{
	asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front(), m_qMessagesOut.front().length()),
		[this](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				// Sending was successful, so we are done with the message
				// and remove it from the queue
				m_qMessagesOut.pop_front();

				// If the queue still has messages in it, then issue the task to
				// send the next messages' header.
				if (!m_qMessagesOut.empty())
				{
					writeData();
				}
			}
			else
			{
				// Sending failed, see WriteHeader() equivalent for description :P
				XPRINT("[", id,"] Write Body Fail.");
				m_socket.close();
			}
		});
}

void CConnection::send(const std::string& msg)
{
	asio::post(m_asioContext,
		[this, msg]()
		{
			// If the queue has a message in it, then we must 
			// assume that it is in the process of asynchronously being written.
			// Either way add the message to the queue to be output. If no messages
			// were available to be written, then start the process of writing the
			// message at the front of the queue.
			bool bWritingMessage = !m_qMessagesOut.empty();
			m_qMessagesOut.push_back(msg);
			if (!bWritingMessage)
			{
				writeData();
			}
		});
}

size_t CConnection::readData()
{
	size_t res = 0;

	if (!m_socket.is_open())
	{
		XPRINT("[CONNECTION] Cannot read data, socket is closet(((");
		return res;
	}

	asio::async_read(m_socket, m_incomMsgBuff, asio::transfer_exactly(3),
		[this](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				// A complete message header has been read, check if this message
				// has a body to follow...
				if (length > 0)
				{
					XPRINT("Data for client has been read succesfully! ", length);
					std::istream s(&this->m_incomMsgBuff);
					s >> std::noskipws;
					std::getline(s, m_msgTemporaryIn);
					addToIncomingMessageQueue();
				}
				else
				{
					XPRINT("Data for client has been read, but it`s empty");
					readData();
				}
			}
			else
			{
				// Reading form the client went wrong, most likely a disconnect
				// has occurred. Close the socket and let the system tidy it up later.
				XPRINT("Read Header Fail.");
				m_socket.close();
			}
		});


	return res;
}
