#include "utils.hpp"

#define PUBLIC "0.0.0.0"
#define LOCAL "127.0.0.1"
#define IP PUBLIC
#define PORT "4444"

int main(int argc, char** argv);
int start_server(Options* options);
void* handle_connections(void* poptions);
void* handle_client(void* pclient_fd);
int parse_arguments(int argc, char** argv, Options* options);
int print_help();

Options options {};
std::vector<int> clients;

int main(int argc, char** argv)
{
	int status;

	options.server = true;	
	strncpy(options.ip, IP, sizeof(options.ip));
	strncpy(options.port, PORT, sizeof(options.port));
	strncpy(options.name, "Server", sizeof(options.name));
	options.encryption = false;
	options.pass = false;
	strncpy(options.password, DEFAULT_PASS, sizeof(options.password));
	options.interactive = false;
	options.max_clients = -1;

	status = parse_arguments(argc, argv, &options);
	if(status)
	{
		print_help();
		goto end;
	}

	status = start_server(&options);
	if(status)
	{
		ERROR_LOG("Failed to start server.\n");
		goto end;
	}

	INFO_LOG("Succesfully started server.\n");

	if(options.interactive)
		start_client(&clients, &options);
	
	pause();

	end:
	return status;
}

int start_server(Options* options)
{
	options->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(options->sockfd < 0)
		return 1;

	sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = inet_addr(options->ip);
	sockaddr.sin_port = htons(atoi(options->port));

	if(bind(options->sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)))
		return 2;

	if(listen(options->sockfd, 1))
		return 3;

	pthread_t connections_thread;
	if(pthread_create(&connections_thread, NULL, handle_connections, (void*)options))
		return 4;
	
	return 0;
}

void* handle_connections(void* poptions)
{
	Options* options = (Options*)poptions;
	sockaddr_in sockaddr;
	socklen_t addrlen = sizeof(sockaddr);

	int connection;
	while((connection = accept(options->sockfd, (struct sockaddr*)&sockaddr, &addrlen)))
	{
		if(connection < 0)
			return 0;

		if((int)(&clients)->size() == options->max_clients)
		{
			ERROR_LOG("Client limit reached. New client has been forbidden to connect.\n");
			close(connection);
			continue;
		}

		pthread_t client_thread;
		if(pthread_create(&client_thread, NULL, handle_client, (void*)(intptr_t)connection))
			return 0;
	}

	return 0;
}

void* handle_client(void* pclient_fd)
{
	int client_fd = (intptr_t)pclient_fd;
	Packet incoming_packet;
	bool first = true;
	char name[20];
	
	int read_size;
	while((read_size = recv(client_fd, &incoming_packet, sizeof(incoming_packet), 0)) > 0)
	{
		int result = handle_incoming_packet(&incoming_packet, &client_fd, &clients, &options);
		if(result == -1)
		{
			ERROR_LOG("Invalid packet recieved.\n");
			break;
		}
		else if(result == -3)
			return 0;
		else if(result > 0)
		{
			ERROR_LOG("Error handling packet recieved.\n");
			break;
		}

		if(first && incoming_packet.type == PacketType::packet_ping)
		{
			first = false;
			strncpy(name, incoming_packet.data.packet_ping.name, sizeof(name));
		}
	}

	if(strlen(name) > 0)
	{
		Packet dc_packet {};
		dc_packet.type = PacketType::packet_disconnect;
		strncpy(dc_packet.data.packet_disconnect.name, name, sizeof(dc_packet.data.packet_disconnect.name));	
		if(options.encryption)
			encrypt(&options, (char*)&dc_packet, sizeof(dc_packet));
		handle_incoming_packet(&dc_packet, &client_fd, &clients, &options);
	}

	close(client_fd);

	return 0;
}

int parse_arguments(int argc, char** argv, Options* options)
{
	extern char* optarg;
	int status = 0;
	int option;
	while((option = getopt(argc, argv, "p:en:ix:h")) != -1)
		switch(option)
		{
			case 'p':
				strncpy(options->port, optarg, sizeof(options->port));
				break;
			case 'e':
				options->encryption = true;
				break;
			case 'n':
				options->max_clients = atoi(optarg);
				break;
			case 'i':
				options->interactive = true;
				break;
			case 'x':
				options->pass = true;
				strncpy(options->password, optarg, sizeof(options->password));
				break;
			case 'h':
				print_help();
				break;	
			case ':':
				status = 1;
				break;
			case '?':
				status = 1;
				break;
		}

	return status;
}

int print_help()
{
	printf(
		"Usage:\n"
			"\t./server\n"
				"\t\t-p (PORT) [port number]\n"
				"\t\t-e [encryption]\n"
				"\t\t-n (NUM_CLIENTS) [max clients]\n"
				"\t\t-i [interactive]\n"
				"\t\t-x (PASS) [pasword]\n"
				"\t\t-h [show help]\n"
	      );
	return 0;
}
