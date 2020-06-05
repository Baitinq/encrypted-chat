struct Options
{
	bool server;
	char name[20];
	char ip[20];
	char port[20];
	bool encryption;
	bool pass;
	char password[20];
	bool interactive;
	int max_clients;
	int sockfd;
};


enum class PacketType
{
	packet_connect,
	packet_disconnect,
	packet_message,
	packet_ping
};

struct PacketConnect
{
	char name[20];
	bool pass;
	char password[20];
};

struct PacketDisconnect
{
	char name[20];
};

struct PacketMessage
{
	int client_fd;
	bool server;
	char name[20];
	char message[200];//make it var length
};

struct PacketPing
{
	char name[20];
};

struct Packet
{
	PacketType type;
	union
	{
		PacketConnect packet_connect;
		PacketDisconnect packet_disconnect;
		PacketMessage packet_message;
		PacketPing packet_ping;
	} data;
	
};
