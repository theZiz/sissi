#include <string.h>
#include <sparrowNet.h>


int main(int argc, char **argv)
{
	spInitNet();
	printf("Sissi test\n");
	spNetIRCServerPointer server = spNetIRCConnectServer("irc.freenode.net",6667,"Sissi","Sissi","Elisabeth");
	if (server)
	{
		char buffer[256];
		spNetResolveHost(server->ip,buffer,256);
		printf("Connecting to: %s\nIP: %i.%i.%i.%i\n",buffer,server->ip.address.ipv4_bytes[0],server->ip.address.ipv4_bytes[1],server->ip.address.ipv4_bytes[2],server->ip.address.ipv4_bytes[3]);
		int i;
		int joined = 0;
		for (i = 1; i <= 300; i++)
		{
			if (spNetIRCServerReady(server) && joined == 0)
			{
				spNetIRCJoinChannel(server,"#hase");
				joined = 1;
			}
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
					printf("(%s) %s: %s\n",name,next->user,next->message);
					*last = next;
					next = next->next;
				}
			}
			if (i == 150)
			{
				spNetIRCSendMessage(server,server->first_channel,"Hallo Welt");
				spNetIRCChannelPointer channel = spNetIRCJoinChannel(server,"Ziz");
				spNetIRCSendMessage(server,channel,"Hallo Ziz");
			}
			if (i == 260)
			{
				spNetIRCChannelPointer mom = server->first_channel;
				while (mom)
				{
					printf("Leaving %s\n",mom->name);
					spNetIRCChannelPointer next = mom->next;
					spNetIRCPartChannel(server,mom);
					mom = next;
				}
				SDL_Delay(1000);
				mom = server->first_channel;
				while (mom)
				{
					printf("Still in %s\n",mom->name);
					mom = mom->next;
				}
			}
			SDL_Delay(100);
		}
		spNetIRCCloseServer(server);
	}
	else
		printf("No connection!\n");
	spQuitNet();	
	return -1;
}
