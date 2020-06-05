#include "utils.hpp"

#define PORT "4444"

int main(int argc, char** argv);
int connect_to_server(Options* options);
void* packet_recieveing_thread(void* poptions);
void* ping_thread(void* poptions);
int parse_arguments(int argc, char** argv, Options* options);
int print_help();

Options options {};

int main(int argc, char** argv)
{
	int status;
	
	char player_name[20] = "Anonymous";
	char num[5];
	sprintf(num, "%d", (int)(time(0) % 9999));
	strncat(player_name, num, sizeof(player_name));

	options.server = false;
	strncpy(options.port, PORT, sizeof(options.port));	
	strncpy(options.name, player_name, sizeof(options.name));	
	options.encryption = false;
	strncpy(options.password, DEFAULT_PASS, sizeof(options.password));	

	status = parse_arguments(argc, argv, &options);
	if(status)
	{
		print_help();
		goto end;
	}

	status = connect_to_server(&options);
	if(status)
	{
		ERROR_LOG("Failed to connect to the server.\n");
		goto end;
	}
	
	INFO_LOG("Succesfully connected to server.\n");

	pthread_t packet_recieveing_thread_id;
	pthread_create(&packet_recieveing_thread_id, NULL, packet_recieveing_thread, (void*)&options);
	
	start_client(NULL, &options);

	end:
	return status;
}

int connect_to_server(Options* options)
{
	options->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(options->sockfd < 0)
		return 1;
	
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = inet_addr(options->ip);
	sockaddr.sin_port = htons(atoi(options->port));

	if(connect(options->sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)))
		return 2;
	
	Packet packet {};
	packet.type = PacketType::packet_connect;
	strncpy(packet.data.packet_connect.name, options->name, sizeof(packet.data.packet_connect.name));
	packet.data.packet_connect.pass = options->pass;
	strncpy(packet.data.packet_connect.password, options->password, sizeof(packet.data.packet_connect.password));

	if(send_packet(&packet, &options->sockfd, options, NULL))
		return 3;
	
	pthread_t ping_thread_id;
	pthread_create(&ping_thread_id, NULL, ping_thread, (void*)options);

	return 0;
}

void* packet_recieveing_thread(void* poptions)
{
	Options* opt = (Options*)poptions;
	Packet incoming_packet {};
 
	int read_size = 0;
	while((read_size = recv(opt->sockfd, &incoming_packet, sizeof(incoming_packet), 0)) > 0)
	{
		int result = handle_incoming_packet(&incoming_packet, &opt->sockfd, NULL, &options);
		if(result)
		{
			ERROR_LOG("Invalid packet recieved.\n");
			exit(1);
		}
	}
	
	return 0;
}

void* ping_thread(void* poptions)
{
	Options* opt = (Options*)poptions;
	while(1)
	{
		Packet ping {};
		ping.type = PacketType::packet_ping;
		strncpy(ping.data.packet_ping.name, opt->name, sizeof(ping.data.packet_ping.name));
		if(send_packet(&ping, &opt->sockfd, opt, NULL))
		{
			ERROR_LOG("Lost connection to server.\n");
			exit(1);
		}
		
		sleep(2);
	}
}

int parse_arguments(int argc, char** argv, Options* options)
{
	extern char* optarg;
	int status = 0;
	bool mand_flag = false;
	int option;
	while((option = getopt(argc, argv, "i:p:en:x:h")) != -1)
		switch(option)
		{
			case 'i':
				strncpy(options->ip, optarg, sizeof(options->ip));
				mand_flag = true;
				break;
			case 'p':
				strncpy(options->port, optarg, sizeof(options->port));
				break;
			case 'e':
				options->encryption = true;
				break;
			case 'n':
				strncpy(options->name, optarg, sizeof(options->name));
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

	if(!mand_flag)
	{
		status = 1;
		ERROR_LOG("Needs ip argument (-i IP)\n");
	}

	return status;
}

int print_help()
{
	printf(
		"Usage:\n"
			"\t./client\n"
				"\t\t-i (IP) [ip address]\n"
				"\t\t-p (PORT) [port number]\n"
				"\t\t-e [encryption]\n"
				"\t\t-n (NAME) [username]\n"
				"\t\t-x (PASS) [password]\n"
				"\t\t-h [show help]\n"
	      );
	return 0;
}
