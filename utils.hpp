#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "encryption.hpp"

#define ERROR_LOG(msg, ...) printf("[!] "); printf(msg, ##__VA_ARGS__); fflush(stdout);
#define INFO_LOG(msg, ...) printf("[+] "); printf(msg, ##__VA_ARGS__); fflush(stdout);
#define PRINT_MSG(name, id, msg, sv) if(sv) printf("[%s]: %s", name, msg); else printf("[%s-%d]: %s", name, id, msg); fflush(stdout);

int start_client(std::vector<int>* clients, Options* options);
int handle_incoming_packet(Packet* packet, int* clientfd, std::vector<int>* clients, Options* options);
int handle_packet_connect(PacketConnect* packet, int* clientfd, std::vector<int>* clients, Options* options);
int handle_packet_disconnect(PacketDisconnect* packet, int* clientfd, std::vector<int>* clients, Options* options);
int handle_packet_message(PacketMessage* packet, int* clientfd, std::vector<int>* clients, Options* options);
int notify_clients(Packet* packet, std::vector<int>* clients, Options* options);
int send_packet(Packet* packet, int* sockfd, Options* options, void* extra_data);
int add_client(int* clientfd, std::vector<int>* clients);
int remove_client(int* clientfd, std::vector<int>* clients);
int enable_echo();
int disable_echo();

int start_client(std::vector<int>* clients, Options* options)
{
	char* stc;
	size_t bytes;
	while(1)
	{
		stc = NULL;
		getline(&stc, &bytes, stdin); //need to make it so it doesnt print to terminal
		//disable_echo();
		Packet packet {};
		packet.type = PacketType::packet_message;
		packet.data.packet_message.server = options->server;
		strncpy(packet.data.packet_message.name, options->name, sizeof(packet.data.packet_message.name));
		strncpy(packet.data.packet_message.message, stc, sizeof(packet.data.packet_message));
		if(options->server)
		{
			if(options->encryption)
				encrypt(options, (char*)&packet, sizeof(packet));
			handle_incoming_packet(&packet, &options->sockfd, clients, options);
		}
		else
			if(send_packet(&packet, &options->sockfd, options, NULL))
			{
				free(stc);
				return 0;
			}

		free(stc);
	}

	return 0;
}

int handle_incoming_packet(Packet* packet, int* sockfd, std::vector<int>* clients, Options* options)
{
	int status = decrypt(options, (char*)packet, sizeof(*packet));
	if(!status)
		switch(packet->type)
		{
			case PacketType::packet_connect:
				status = handle_packet_connect(&packet->data.packet_connect, sockfd, clients, options);
				break;
			case PacketType::packet_disconnect:
				status = handle_packet_disconnect(&packet->data.packet_disconnect, sockfd, clients, options);
				break;
			case PacketType::packet_message:
				status = handle_packet_message(&packet->data.packet_message, sockfd, options->server ? clients : NULL, options);
				break;
			case PacketType::packet_ping:
				status = -2; //dont notify
				break;
			default:
				status = -1;
				break;
		}

	if(options->server && !status)
		status = notify_clients(packet, clients, options);

	return status;
}

int handle_packet_connect(PacketConnect* packet, int* clientfd, std::vector<int>* clients, Options* options)
{
	int status = 0;
	if(options->server)
	{
		if(options->pass && (!packet->pass || strcmp(options->password, packet->password)))
		{
			status = -3;
			INFO_LOG("Player tried to join with an incorrect password.\n");
			close(*clientfd);
			goto end;
		}
		
		status = add_client(clientfd, clients);
	}

	INFO_LOG("%s joined the chat!\n", packet->name);	


	end:
	return status;
}

int handle_packet_disconnect(PacketDisconnect* packet, int* clientfd, std::vector<int>* clients, Options* options)
{
	int status = 0;
	if(options->server)
		status = remove_client(clientfd, clients);
	
	INFO_LOG("%s left the chat!\n", packet->name);	
	
	return status;
}

int handle_packet_message(PacketMessage* packet, int* clientfd, std::vector<int>* clients, Options* options)
{
	if(options->server)
	{
		packet->client_fd = *clientfd;
		std::vector<int>::iterator pos = std::find(clients->begin(), clients->end(), *clientfd);
		if(*clientfd != options->sockfd && pos == clients->end())
			return 1;
	}
	
	packet->client_fd = *clientfd;

	if(!options->server || options->interactive)
	{
		PRINT_MSG(packet->name, packet->client_fd, packet->message, packet->server);
	}

	return 0;
}

int notify_clients(Packet* packet, std::vector<int>* clients, Options* options)
{
	for(int clientfd : *clients)
		if(send_packet(packet, &clientfd, options, NULL))
			return 1;

	return 0;
}

int send_packet(Packet* packet, int* sockfd, Options* options, void* extra_data)
{
	int status = encrypt(options, (char*)packet, sizeof(*packet));
	if(status)
		return status;
	if(send(*sockfd, (const char*)packet, sizeof(*packet), 0) < 0)
		status = 1;
	status = decrypt(options, (char*)packet, sizeof(*packet));
	
	return status;
}

int add_client(int* clientfd, std::vector<int>* clients)
{
	std::vector<int>::iterator pos = std::find(clients->begin(), clients->end(), *clientfd);
	if(pos == clients->end())
	{
		clients->push_back(*clientfd);
		return 0;
	}

	return 1;
}

int remove_client(int* clientfd, std::vector<int>* clients)
{
	std::vector<int>::iterator pos = std::find(clients->begin(), clients->end(), *clientfd);
	if(pos != clients->end())
	{
		clients->erase(pos);
		return 0;
	}

	close(*clientfd);

	return 1;
}

int enable_echo()
{
	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag |= ECHO;
	tcsetattr(0, TCSAFLUSH, &term);

	return 0;
}
	
int disable_echo()
{
	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag &= ~ECHO;
	tcsetattr(0, TCSAFLUSH, &term);

	return 0;
}
