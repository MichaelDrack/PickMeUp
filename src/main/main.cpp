#include <iostream>
#include <string>

#include <unordered_map>
#include "server/server.h"


struct client_desc
{
	uint32_t uID;

	std::string name;
	std::string surname;
};

class CListener : public CServer
{
	public:
		CListener(uint32_t port) : CServer(port)
		{
		}

		std::unordered_map<uint32_t, client_desc> m_mapClients;
	protected:
		bool OnClientConnect(std::shared_ptr<CConnection> client) override
		{
			// For now we will allow all
			return true;
		}

		void OnClientValidated(std::shared_ptr<CConnection> client) override
		{
			// Client passed validation check, so send them a message informing
			// them they can continue to communicate
			std::string msg = "Hello, I am a server, please wellcome!";
			client->send(msg);
		}

		void OnClientDisconnect(std::shared_ptr<CConnection> client) override
		{
			if (client)
			{
				if (m_mapClients.find(client->getID()) == m_mapClients.end())
				{
					// client never added to roster, so just let it disappear
				}
				else
				{
					auto& pd = m_mapClients[client->getID()];
					std::cout << "[UNGRACEFUL REMOVAL]:" + std::to_string(pd.uID) + "\n";
					m_mapClients.erase(client->getID());
				}
			}
		}

		void OnMessage(std::shared_ptr<CConnection> client, std::string& msg) override
		{
			std::cout << "Hey! we received a message: " << msg << std::endl;
			client->send("Great! I got this!");
		}
	private:
};

int main(int argc, char** argv)
{
	std::cout << "The program is running!" << std::endl;

	CListener server(5566);
	server.start();

	while(1)
		server.update();

	return 0;
}
