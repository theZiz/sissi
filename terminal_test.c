 /* This file is part of Sissi.
  * Sissi is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 2 of the License, or
  * (at your option) any later version.
  * 
  * Sissi is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with glutexto.  If not, see <http://www.gnu.org/licenses/>
  * 
  * For feedback and questions about my Files and Projects please mail me,
  * Alexander Matthes (Ziz) , zizsdl_at_googlemail.com */
#include <string.h>
#include <sparrowNet.h>

int starts_with(char* stack,char* needle)
{
	int i = 0;
	while (needle[i] != 0)
	{
		if (stack[i] != needle[i])
			return 0;
		i++;
	}
	return 1;
}

int main(int argc, char **argv)
{
	printf("Sissi Terminal Test\n");
	if (argc < 4)
	{
		printf("Usage: terminal_test server port nick\n");
		return 1;
	}
	char* name = argv[1];
	int port = atoi(argv[2]);
	char* nick = argv[3];
	printf("\033[1mConnecting to %s:%i as %s\033[0m\n",name,port,nick);
	spInitNet();
	spNetIRCServerPointer server = spNetIRCConnectServer(name,port,nick,"Sissi","Elisabeth","*");
	if (server == NULL)
	{
		printf("\033[1mConnection failed...\033[0m\n");
		spQuitNet();
		return 1;
	}
	char buffer[256];
	spNetResolveHost(server->ip,buffer,256);
	printf("\033[1mConnecting to %s with IP %i.%i.%i.%i\033[0m\n",buffer,server->ip.address.ipv4_bytes[0],server->ip.address.ipv4_bytes[1],server->ip.address.ipv4_bytes[2],server->ip.address.ipv4_bytes[3]);
	printf("Commands:\n");
	printf("update - show what happens since the last time\n");
	printf("join channel - join a channel\n");
	printf("part channel - part a channel\n");
	printf("users channel - users of a channel\n");
	printf("msg nick/channel message - send a message to the world\n");
	printf("server message - send a message directly to the server\n");
	printf("quit - quit\n");
	while (1)
	{
		printf("Enter a command (update,join,part,users,msg,server,quit)\n");
		char buffer[4096] = "";
		char* result = fgets(buffer,4096,stdin);
		strchr(buffer,'\n')[0] = 0;
		if (starts_with(buffer,"update"))
		{
			int first_run = 1;
			spNetIRCChannelPointer channel = server->first_channel;
			while (channel || first_run)
			{
				char* name;
				spNetIRCMessagePointer next = NULL;
				spNetIRCMessagePointer* first;
				spNetIRCMessagePointer* last;
				if (first_run)
				{
					name = server->name;
					last = &(server->last_read_message);
					first = &(server->first_message);
					first_run = 0;
				}
				else
				{
					name = channel->name;
					last = &(channel->last_read_message);
					first = &(channel->first_message);
					channel = channel->next;
				}
				if (*last)
					next = (*last)->next;
				else
				if (*first)
					next = (*first);
				while (next)
				{
					printf("\033[4m(%s)\033[0m \033[1m%s: %s\033[0m\n",name,next->user,next->message);
					*last = next;
					next = next->next;
				}
			}
		}
		else
		if (starts_with(buffer,"quit"))
			break;
		else
		if (starts_with(buffer,"join "))
		{
			char* destiny = strchr(buffer,' ');
			destiny++;
			spNetIRCJoinChannel(server,destiny);
		}
		else
		if (starts_with(buffer,"part "))
		{
			char* destiny = strchr(buffer,' ');
			destiny++;
			spNetIRCChannelPointer channel = server->first_channel;
			while (channel)
			{
				if (strcmp(channel->name,destiny) == 0)
					break;
				channel = channel->next;
			}
			if (channel)
				spNetIRCPartChannel(server,channel);
		}
		else
		if (starts_with(buffer,"users "))
		{
			char* destiny = strchr(buffer,' ');
			destiny++;
			spNetIRCChannelPointer channel = server->first_channel;
			while (channel)
			{
				if (strcmp(channel->name,destiny) == 0)
					break;
				channel = channel->next;
			}
			if (channel)
			{
				printf("\033[1mUsers of %s:",destiny);
				spNetIRCNickPointer nick = channel->first_nick;
				while (nick)
				{
					printf(" %s",nick->name);
					if (nick->next)
						printf(",");
					nick = nick->next;
				}
				printf("\033[0m\n");
			}
		}
		else
		if (starts_with(buffer,"msg "))
		{
			char* destiny = strchr(buffer,' ');
			destiny++;
			char* message = strchr(destiny,' ');
			if (message)
			{
				message[0] = 0;
				message++;
				spNetIRCChannelPointer channel = spNetIRCJoinChannel(server,destiny);
				while (channel->status == 0)
				{
					SDL_Delay(100);
				}
				spNetIRCSendMessage(server,channel,message);
			}
			else
				printf("\033[31mwrong command!\033[0m\n");
		}
		else
		if (starts_with(buffer,"server "))
		{
			char* message = strchr(buffer,' ');
			message++;
			spNetIRCSendMessage(server,NULL,message);
		}
		else
			printf("\033[31mwrong command!\033[0m\n");
	}
	printf("Finishing...\n");
	spNetIRCCloseServer(server);
	spQuitNet();	
	return -1;
}
