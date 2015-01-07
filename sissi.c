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


#define GCW_FEELING

#if defined GCW_FEELING && defined DESKTOP
	#define TESTING
	#define GCW
	#undef DESKTOP
#endif

#include <sparrow3d.h>

#if defined GCW_FEELING && defined TESTING
	#define DESKTOP
	#undef GCW
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FONT_LOCATION "./data/PixelManiaConden.ttf"
#define FONT_SIZE_BIG 32
#define FONT_SIZE 16
#define FONT_SIZE_CHAT 16
#define FONT_SIZE_SMALL 12
#define MAIN_COLOR spGetRGB(255,255,255)
#define SEC_COLOR spGetRGB(192,192,192)
#define BG1_COLOR spGetRGB(0,0,0)
#define BG2_COLOR spGetRGB(64,64,64)

spFontPointer font_big = NULL;
spFontPointer font = NULL;
spFontPointer font_chat = NULL;
spFontPointer font_small = NULL;

SDL_Surface* screen = NULL;

void resize(Uint16 w,Uint16 h);

typedef struct sWindow *pWindow;
typedef struct sWindow {
	int kind;//0 server, 1 channel;
	spNetIRCMessagePointer* first_message;
	spNetIRCMessagePointer* last_read_message;
	union
	{
		struct
		{
			char name[256];
			char port[256];
			char nickname[256];
			char username[256];
			char realname[256];
			int selection;
			spNetIRCServerPointer server;
		} server;
		struct
		{
			char name[256];
			spNetIRCChannelPointer channel;
			int selected_nick;
		} channel;
	} data;
	SDL_Surface* message_window;
	SDL_Surface* options_window;
	spTextBlockPointer block;
	int scroll;
	char message[512];
	pWindow next;
} tWindow;

tWindow serverWindow;
pWindow momWindow = &serverWindow;
int showMessage = 0;
int blinkCounter = 0;

spConfigPointer config;

void save_config()
{
	sprintf(spConfigGetString(config,"server_name","irc.freenode.net"),"%s",serverWindow.data.server.name);
	sprintf(spConfigGetString(config,"server_port","6667"),"%s",serverWindow.data.server.port);
	sprintf(spConfigGetString(config,"nickname","Sissiuser"),"%s",serverWindow.data.server.nickname);
	sprintf(spConfigGetString(config,"username","Sissiuser"),"%s",serverWindow.data.server.username);
	sprintf(spConfigGetString(config,"realname","Elisabeth Amalie Eugenie"),"%s",serverWindow.data.server.realname);
	spConfigWrite(config);
}

void draw_keyboard()
{
	spFontDrawMiddle(screen->w/2,screen->h-spGetVirtualKeyboard()->h-font_small->maxheight,0,SP_PAD_NAME" select [o] enter [c] remove [3] space [4] shift [R] exit",font_small);
	spBlitSurface(screen->w/2,screen->h-spGetVirtualKeyboard()->h/2,0,spGetVirtualKeyboard());
}

void update_message_window(pWindow window)
{
	spSelectRenderTarget(window->message_window);
	spClearTarget( BG1_COLOR );
	if (window->kind == 0)
		spFontDrawMiddle(screen->w/2,0,0,"[l]+[v] options [l]+[<][>] switch tab [l]+[^] scroll [B] reset scroll",font_small);
	else
		spFontDrawMiddle(screen->w/2,0,0,"[l]+[v] nick list [l]+[<][>] switch tab [l]+[^] scroll [B] reset scroll",font_small);
	if (window->kind == 0 && serverWindow.data.server.server == NULL)
		spFontDrawMiddle(screen->w/2,font_small->maxheight*3,0,"Can't connect.",font_big);
	else
	{
		if (window->block)
			spFontDrawTextBlock(left,2,font_small->maxheight+font->maxheight,0,window->block,screen->h-spGetVirtualKeyboard()->h-2*font_small->maxheight-2*font->maxheight,(window->scroll == -1)?SP_ONE:((window->scroll*SP_ONE+1)/window->block->line_count),font_chat);
		if (window->kind == 0)
			spFontDrawMiddle(screen->w/2,font_small->maxheight,0,window->data.server.name,font);
		else
			spFontDrawMiddle(screen->w/2,font_small->maxheight,0,window->data.channel.name,font);
		int w = spFontDrawRight(screen->w-2,screen->h-spGetVirtualKeyboard()->h-font->maxheight-font_small->maxheight+(font->maxheight-font_small->maxheight)/2,0,"[r] Send",font_small);
		spRectangle(screen->w/2-w/2-3,screen->h-spGetVirtualKeyboard()->h-font->maxheight/2-font_small->maxheight,0,screen->w-w-6,font->maxheight*3/4,BG2_COLOR);
		
		if (spFontWidth(window->message,font) < screen->w-w-6)
		{
			if (blinkCounter < 500 && spIsKeyboardPolled())
				spFontDraw(2+spFontWidth(window->message,font),screen->h-spGetVirtualKeyboard()->h-font->maxheight-font_small->maxheight,0,"|",font);	
			spFontDraw(2,screen->h-spGetVirtualKeyboard()->h-font->maxheight-font_small->maxheight,0,window->message,font);
		}
		else
		{
			if (blinkCounter < 500 && spIsKeyboardPolled())
				spFontDraw(2+screen->w-w-6,screen->h-spGetVirtualKeyboard()->h-font->maxheight-font_small->maxheight,0,"|",font);	
			spFontDrawRight(2+screen->w-w-6,screen->h-spGetVirtualKeyboard()->h-font->maxheight-font_small->maxheight,0,window->message,font);
		}
		draw_keyboard();
	}
}

void update_options_window(pWindow window)
{
	spSelectRenderTarget(window->options_window);
	spClearTarget( BG1_COLOR );
	if (window->kind == 0)
	{
		spFontDrawMiddle(screen->w/2,0,0,"[l]+[^] apply/server view [l]+[<][>] switch tab [B] select [r] join",font_small);
		
		spFontDrawMiddle(screen->w/2,font_small->maxheight*3,0,"Server config",font_big);
		
		int w = spFontWidth("-> user name: ",font)+2;
		int y = font_small->maxheight*3+font_big->maxheight+font_small->maxheight/2;
		spFontDraw(2,y+(font->maxheight+font_small->maxheight/2)*serverWindow.data.server.selection,0,"->",font);
		if (blinkCounter < 500 && spIsKeyboardPolled())
			spFontDraw(w+spFontWidth(spGetInput()->keyboard.buffer,font),y+(font->maxheight+font_small->maxheight/2)*serverWindow.data.server.selection,0,"|",font);

		spFontDrawRight(w,y,0,"address: ",font);
		spFontDraw(w,y,0,serverWindow.data.server.name,font);
		y+=font->maxheight+font_small->maxheight/2;
		spFontDrawRight(w,y,0,"port: ",font);
		spFontDraw(w,y,0,serverWindow.data.server.port,font);
		y+=font->maxheight+font_small->maxheight/2;
		spFontDrawRight(w,y,0,"nick name: ",font);
		spFontDraw(w,y,0,serverWindow.data.server.nickname,font);
		y+=font->maxheight+font_small->maxheight/2;
		spFontDrawRight(w,y,0,"user name: ",font);
		spFontDraw(w,y,0,serverWindow.data.server.username,font);
		y+=font->maxheight+font_small->maxheight/2;
		spFontDrawRight(w,y,0,"real name: ",font);
		spFontDraw(w,y,0,serverWindow.data.server.realname,font);
		
		draw_keyboard();
	}
	else
	{
		spFontDrawMiddle(screen->w/2,0,0,"[l]+[^] channel [l]+[<][>] tab [v][^] select [o] query [B] leave [r] join",font_small);

		int c = 0;
		spNetIRCNickPointer nick = window->data.channel.channel->first_nick;
		while (nick)
		{
			c++;
			nick = nick->next;
		}
		if (c == 0)
			window->data.channel.selected_nick = 0;
		else
		{
			while (window->data.channel.selected_nick < 0)
				window->data.channel.selected_nick += c;
			while (window->data.channel.selected_nick >= c)
				window->data.channel.selected_nick -= c;
		}
		char buffer[256];
		sprintf(buffer,"%i user online in %s:",c,window->data.channel.name);
		spFontDrawMiddle(screen->w/2,font_small->maxheight,0,buffer,font);

		nick = window->data.channel.channel->first_nick;
		int y = font_small->maxheight+font->maxheight;
		int max = screen->h-2*font_small->maxheight-font->maxheight;
		int d = font->maxheight*4/5;
		if (c > max/d)
			y += -d*window->data.channel.selected_nick + (max*window->data.channel.selected_nick/c)/d*d;
		c = 0;
		while (nick && y < screen->h-font->maxheight)
		{
			if (y >= font_small->maxheight+font->maxheight)
			{
				if (c == window->data.channel.selected_nick)
					sprintf(buffer,"-> %s <-",nick->name);
				else
					sprintf(buffer,"%s",nick->name);
				spFontDrawMiddle(screen->w/2,y,0,buffer,font);
			}
			y += font->maxheight*4/5;
			c++;
			nick = nick->next;
		}
	}
}

void handle_keyboard_buttons()
{
	if (spIsKeyboardPolled() == 0)
		return;
	if (spGetInput()->button[SP_PRACTICE_CANCEL_NOWASD])
	{
		spGetInput()->button[SP_PRACTICE_CANCEL_NOWASD] = 0;
		if (spGetInput()->keyboard.pos > 0)
		{
			spGetInput()->keyboard.pos--;
			spGetInput()->keyboard.buffer[spGetInput()->keyboard.pos] = 0;
		}
	}
	if (spGetInput()->button[SP_PRACTICE_3_NOWASD])
	{
		spGetInput()->button[SP_PRACTICE_3_NOWASD] = 0;
		if (spGetInput()->keyboard.pos < spGetInput()->keyboard.len-1)
		{
			spGetInput()->keyboard.buffer[spGetInput()->keyboard.pos] = ' ';
			spGetInput()->keyboard.pos++;
			spGetInput()->keyboard.buffer[spGetInput()->keyboard.pos] = 0;
		}
	}
	if (spGetInput()->button[SP_PRACTICE_4_NOWASD])
	{
		spGetInput()->button[SP_PRACTICE_4_NOWASD] = 0;
		spSetVirtualKeyboardShiftState(1-spGetVirtualKeyboardShiftState());
	}
}

char upper_case(char c)
{
	if (c >= 'a' && c < 'z')
		c -= 32;
	return c;
}

int starts_with(char* stack,char* needle)
{
	int i = 0;
	while (needle[i] != 0)
	{
		if (upper_case(stack[i]) != upper_case(needle[i]))
			return 0;
		i++;
	}
	return 1;
}

pWindow create_channel_window(spNetIRCChannelPointer channel)
{
	pWindow new_window = (pWindow)malloc(sizeof(tWindow));
	new_window->kind = 1;
	new_window->first_message = &(channel->first_message);
	new_window->last_read_message = &(channel->last_read_message);
	sprintf(new_window->data.channel.name,"%s",channel->name);
	new_window->data.channel.channel = channel;
	new_window->data.channel.selected_nick = 0;
	new_window->message_window = spCreateSurface(screen->w,screen->h);
	new_window->options_window = spCreateSurface(screen->w,screen->h);
	new_window->block = NULL;
	new_window->scroll = -1;
	new_window->message[0] = 0;
	new_window->next = serverWindow.next;
	serverWindow.next = new_window;
	return new_window;
}

int button_pressed = 0;

void calc_message_window(pWindow window,int steps)
{
	if (spGetInput()->button[SP_BUTTON_SELECT_NOWASD])
	{
		spGetInput()->button[SP_BUTTON_SELECT_NOWASD] = 0;
		window->scroll = -1;
	}
	if (spGetInput()->button[SP_BUTTON_L_NOWASD] && spGetInput()->axis[1] < 0)
	{
		if (window->scroll == -1)
			window->scroll = window->block->line_count;
		if (button_pressed == 0)
		{
			window->scroll--;
			button_pressed = 300;
		}
		else
		{
			button_pressed-=steps;
			if (button_pressed <= 0)
			{
				window->scroll--;
				button_pressed += 50;
			}
		}
		if (window->scroll < 0)
			window->scroll = 0;
	}
	else
		button_pressed = 0;
	if (spGetInput()->button[SP_BUTTON_R_NOWASD])
	{
		spGetInput()->button[SP_BUTTON_R_NOWASD] = 0;
		if (window->message[0] == '/')
		{
			if (starts_with(window->message,"/join "))
			{
				char *destiny = strchr(window->message,' ');
				destiny++;
				spNetIRCChannelPointer channel = spNetIRCJoinChannel(serverWindow.data.server.server,destiny);
				if (channel)
				{
					spStopKeyboardInput();
					momWindow = create_channel_window(channel);
					showMessage = 1;
					spPollKeyboardInput(momWindow->message,512,SP_PRACTICE_OK_NOWASD_MASK);
					return;
				}
			}
			else
				spNetIRCSendMessage(serverWindow.data.server.server,NULL,&(window->message[1]));
		}
		else
		if (window->kind)
			spNetIRCSendMessage(serverWindow.data.server.server,window->data.channel.channel,window->message);
		spStopKeyboardInput();
		window->message[0] = 0;
		spPollKeyboardInput(window->message,512,SP_PRACTICE_OK_NOWASD_MASK);
	}
	handle_keyboard_buttons();
	if (window->last_read_message == NULL || window->first_message == NULL || *(window->first_message) == NULL)
		return;
	char buffer[2048],time_buffer[128];
    time_t t = time(NULL);
    struct tm *ti = gmtime(&t);
    strftime(time_buffer,2048,"%X",ti);
    if (*(window->last_read_message) == NULL)
	{
		if (window->kind == 0)
			sprintf(buffer,"%s: %s",time_buffer,(*(window->first_message))->message);
		else
			sprintf(buffer,"%s %s: %s",time_buffer,(*(window->first_message))->user,(*(window->first_message))->message);
		window->block = spCreateTextBlock(buffer,screen->w-4,font_chat);
		*(window->last_read_message) = *(window->first_message);
	}
	if (*(window->last_read_message))
		while ((*(window->last_read_message))->next)
		{
			spNetIRCMessagePointer next = (*(window->last_read_message))->next;
			if (window->kind == 0)
				sprintf(buffer,"%s: %s",time_buffer,next->message);
			else
				sprintf(buffer,"%s %s: %s",time_buffer,next->user,next->message);
			spTextBlockPointer temp = spCreateTextBlock(buffer,screen->w-4,font_chat);
			int lc = window->block->line_count + temp->line_count;
			spTextLinePointer copyLine = (spTextLinePointer)malloc(lc*sizeof(spTextLine));
			memcpy(copyLine,window->block->line,window->block->line_count*sizeof(spTextLine));
			memcpy(&copyLine[window->block->line_count],temp->line,temp->line_count*sizeof(spTextLine));
			free(window->block->line);
			window->block->line = copyLine;
			window->block->line_count = lc;
			free(temp);
			*(window->last_read_message) = next;
		}
}

char join[256] = "";

void draw_join()
{
	spSelectRenderTarget(spGetWindowSurface());
	spClearTarget( 0 );
	spSetBlending(SP_ONE/4);
	spBlitSurface(screen->w/2,screen->h/2,0,momWindow->options_window);
	spSetBlending(SP_ONE);
	spFontDrawMiddle(screen->w/2,0,0,"[r] Join [R] Cancel",font_small);
	spFontDrawMiddle(screen->w/2,font_small->maxheight*3,0,"Join a channel",font_big);
	
	if (blinkCounter < 500 && spIsKeyboardPolled())
		spFontDraw(screen->w/2+spFontWidth(join,font_big)/2,screen->h/2-font_big->maxheight,0,"|",font_big);
	spFontDrawMiddle(screen->w/2,screen->h/2-font_big->maxheight,0,join,font_big);
	
	draw_keyboard();
	
	spFlip();
}

int calc_join(Uint32 steps)
{
	blinkCounter+=steps;
	while (blinkCounter > 1000)
		blinkCounter-=1000;
	handle_keyboard_buttons();
	if (spGetInput()->button[SP_BUTTON_R_NOWASD])
	{
		spGetInput()->button[SP_BUTTON_R_NOWASD] = 0;
		return 2;
	}
	if (spGetInput()->button[SP_BUTTON_START_NOWASD])
	{
		spGetInput()->button[SP_BUTTON_START_NOWASD] = 0;
		return 1;
	}
	return 0;
}


int join_channel()
{
	if (serverWindow.data.server.server == NULL)
		return 0;
	sprintf(join,"#");
	spPollKeyboardInput(join,256,SP_PRACTICE_OK_NOWASD_MASK);
	spNetIRCChannelPointer channel = NULL;
	if (spLoop( draw_join, calc_join, 10, resize, NULL ) == 2)
	{
		channel = spNetIRCJoinChannel(serverWindow.data.server.server,join);
	}
	spStopKeyboardInput();
	if (channel)
	{
		momWindow = create_channel_window(channel);
		showMessage = 1;
		spPollKeyboardInput(momWindow->message,512,SP_PRACTICE_OK_NOWASD_MASK);
		return 1;
	}
	return 0;
}

void start_keyboard_server_options()
{
	switch (serverWindow.data.server.selection)
	{
		case 0:
			spPollKeyboardInput(serverWindow.data.server.name,256,SP_PRACTICE_OK_NOWASD_MASK);
			break;
		case 1:
			spPollKeyboardInput(serverWindow.data.server.port,256,SP_PRACTICE_OK_NOWASD_MASK);
			break;
		case 2:
			spPollKeyboardInput(serverWindow.data.server.nickname,256,SP_PRACTICE_OK_NOWASD_MASK);
			break;
		case 3:
			spPollKeyboardInput(serverWindow.data.server.username,256,SP_PRACTICE_OK_NOWASD_MASK);
			break;
		case 4:
			spPollKeyboardInput(serverWindow.data.server.realname,256,SP_PRACTICE_OK_NOWASD_MASK);
			break;
	}
}

void leave_channel(pWindow window)
{
	if (serverWindow.data.server.server)
		spNetIRCPartChannel(serverWindow.data.server.server,window->data.channel.channel);
	if (window->block)
		spDeleteTextBlock(window->block);
	//find before
	pWindow before = &serverWindow;
	while (before->next != window)
		before = before->next;
	before->next = window->next;
	if (momWindow == window)
	{
		showMessage = 1;
		momWindow = momWindow->next;
	}
	if (momWindow == NULL)
		momWindow = &serverWindow;
	free(window);
}

void calc_options_window(pWindow window,int steps)
{
	if (spGetInput()->button[SP_BUTTON_SELECT_NOWASD])
	{
		spGetInput()->button[SP_BUTTON_SELECT_NOWASD] = 0;
		if (momWindow->kind == 0)
		{
			momWindow->data.server.selection = (momWindow->data.server.selection+1)%5;
			spStopKeyboardInput();
			start_keyboard_server_options();
		}
		else
		{
			leave_channel(momWindow);
		}
	}
	if (momWindow->kind && spGetInput()->button[SP_PRACTICE_OK_NOWASD])
	{
		spGetInput()->button[SP_PRACTICE_OK_NOWASD] = 0;
		spNetIRCNickPointer nick = momWindow->data.channel.channel->first_nick;
		int i = 0;
		while (nick)
		{
			if (i == momWindow->data.channel.selected_nick)
				break;
			i++;
			nick = nick->next;
		}
		if (nick)
		{
			spNetIRCChannelPointer channel = spNetIRCJoinChannel(serverWindow.data.server.server,nick->name);
			momWindow = create_channel_window(channel);
			showMessage = 1;
			spPollKeyboardInput(momWindow->message,512,SP_PRACTICE_OK_NOWASD_MASK);
		}
		
	}
	if (spGetInput()->button[SP_BUTTON_R_NOWASD])
	{
		spGetInput()->button[SP_BUTTON_R_NOWASD] = 0;
		spStopKeyboardInput();
		if (join_channel())
			return;
		start_keyboard_server_options();
	}
	if (window->kind == 0)
		handle_keyboard_buttons();
	else
	{
		if (spGetInput()->axis[1] > 0)
		{
			if (button_pressed == 0)
			{
				window->data.channel.selected_nick++;
				button_pressed = 300;
			}
			else
			{
				button_pressed-=steps;
				if (button_pressed <= 0)
				{
					window->data.channel.selected_nick++;
					button_pressed += 100;
				}
			}
		}
		else
		if (spGetInput()->axis[1] < 0)
		{
			if (button_pressed == 0)
			{
				window->data.channel.selected_nick--;
				button_pressed = 300;
			}
			else
			{
				button_pressed-=steps;
				if (button_pressed <= 0)
				{
					window->data.channel.selected_nick--;
					button_pressed += 100;
				}
			}
		}
		else
			button_pressed = 0;
	}
}

char oldName[256];
char oldPort[256];
char oldNickname[256];
char oldUsername[256];
char oldRealname[256];

#define TRANSIT_TIME 300
int transit = 0; //1 left, 2 up, 3 right, 4 down
int transit_counter = 0;

pWindow goalWindow;

void draw()
{
	spSelectRenderTarget(spGetWindowSurface());
	switch (transit)
	{
		case 0:
			if (showMessage)
				spBlitSurface(screen->w/2,screen->h/2,0,momWindow->message_window);
			else
				spBlitSurface(screen->w/2,screen->h/2,0,momWindow->options_window);
			break;
		case 1:
			if (showMessage)
			{
				spBlitSurface(screen->w*1/2-transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,goalWindow->message_window);
				spBlitSurface(screen->w*3/2-transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,momWindow->message_window);
			}
			else
			{
				spBlitSurface(screen->w*1/2-transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,goalWindow->options_window);
				spBlitSurface(screen->w*3/2-transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,momWindow->options_window);
			}
			break;
		case 2:
			spBlitSurface(screen->w/2,screen->h*1/2-transit_counter*screen->h/TRANSIT_TIME,0,momWindow->message_window);
			spBlitSurface(screen->w/2,screen->h*3/2-transit_counter*screen->h/TRANSIT_TIME,0,momWindow->options_window);
			break;
		case 3:
			if (showMessage)
			{
				spBlitSurface(screen->w*-1/2+transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,momWindow->message_window);
				spBlitSurface(screen->w* 1/2+transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,goalWindow->message_window);
			}
			else
			{
				spBlitSurface(screen->w*-1/2+transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,momWindow->options_window);
				spBlitSurface(screen->w* 1/2+transit_counter*screen->w/TRANSIT_TIME,screen->h/2,0,goalWindow->options_window);
			}
			break;
		case 4:
			spBlitSurface(screen->w/2,screen->h*-1/2+transit_counter*screen->h/TRANSIT_TIME,0,momWindow->message_window);
			spBlitSurface(screen->w/2,screen->h* 1/2+transit_counter*screen->h/TRANSIT_TIME,0,momWindow->options_window);
			break;
	}
	spFlip();
}

void start_options_window()
{
	if (momWindow->kind == 0)
	{
		start_keyboard_server_options();
		sprintf(oldName,"%s",serverWindow.data.server.name);
		sprintf(oldPort,"%s",serverWindow.data.server.port);
		sprintf(oldNickname,"%s",serverWindow.data.server.nickname);
		sprintf(oldUsername,"%s",serverWindow.data.server.username);
		sprintf(oldRealname,"%s",serverWindow.data.server.realname);
	}
	showMessage = 0;	
}

void start_message_window()
{
	if (momWindow->kind == 0 && serverWindow.data.server.server)
	{
		if (strcmp(oldName,serverWindow.data.server.name) || strcmp(oldPort,serverWindow.data.server.port))
		{
			//part all channels
			pWindow window = serverWindow.next;
			while (window)
			{
				pWindow next = window->next;
				leave_channel(window);
				window = next;
			}
			spNetIRCCloseServer(serverWindow.data.server.server);
			serverWindow.data.server.server = NULL;
		}
		if (strcmp(oldNickname,serverWindow.data.server.nickname))
		{
			char buffer[1024];
			sprintf(buffer,"NICK %s",serverWindow.data.server.nickname);
			spNetIRCSend(serverWindow.data.server.server,buffer);
		}
	}
	if (momWindow->kind == 0 && serverWindow.data.server.server == NULL)
	{
		serverWindow.data.server.server = spNetIRCConnectServer(serverWindow.data.server.name,atoi(serverWindow.data.server.port),serverWindow.data.server.nickname,serverWindow.data.server.username,serverWindow.data.server.realname);
		if (serverWindow.data.server.server)
		{
			momWindow->first_message = &(serverWindow.data.server.server->first_message);
			momWindow->last_read_message = &(serverWindow.data.server.server->last_read_message);
			if (serverWindow.block)
			{
				spDeleteTextBlock(serverWindow.block);
				serverWindow.block = NULL;
				serverWindow.scroll = -1;
			}
		}
	}
	if (momWindow->kind == 0)
		save_config();
	if (momWindow->kind || serverWindow.data.server.server)
		spPollKeyboardInput(momWindow->message,512,SP_PRACTICE_OK_NOWASD_MASK);
	showMessage = 1;
}

int calc(Uint32 steps)
{
	if (spGetInput()->button[SP_BUTTON_START_NOWASD])
		return 1;
	if (transit)
	{
		transit_counter-=steps;
		if (transit_counter <= 0)
		{
			switch (transit)
			{
				case 1:case 3:
					momWindow = goalWindow;
					if (showMessage)
						start_message_window();
					else
						start_options_window();
					break;
				case 2:
					start_message_window();
					break;
				case 4:
					start_options_window();
					break;
			}
			transit = 0;
		}
		return 0;
	}
	//observer!
	//is a channel gone?
	if (serverWindow.data.server.server)
	{
		pWindow window = serverWindow.next;
		while (window)
		{
			pWindow next = window->next;
			spNetIRCChannelPointer channel = serverWindow.data.server.server->first_channel;
			while (channel)
			{
				if (channel == window->data.channel.channel)
					break;
				channel = channel->next;
			}
			if (channel == NULL) //not found!
				leave_channel(window);
			window = next;
		}
		//is a new channel available?
		spNetIRCChannelPointer channel = serverWindow.data.server.server->first_channel;
		while (channel)
		{
			pWindow window = serverWindow.next;
			while (window)
			{	
				if (window->data.channel.channel == channel)
					break;
				window = window->next;
			}
			if (window == NULL) //not found!
			{
				spStopKeyboardInput();
				momWindow = create_channel_window(channel);
				showMessage = 1;
				spPollKeyboardInput(momWindow->message,512,SP_PRACTICE_OK_NOWASD_MASK);
			}
			channel = channel->next;
		}
	}
	
	blinkCounter+=steps;
	while (blinkCounter > 1000)
		blinkCounter-=1000;
	if (showMessage)
	{
		if (spGetInput()->button[SP_BUTTON_L_NOWASD] && spGetInput()->axis[1] > 0)
		{
			spStopKeyboardInput();
			update_options_window(momWindow);
			update_message_window(momWindow);
			transit = 4;
			transit_counter = TRANSIT_TIME;
			return 0;
		}
	}
	else
	{
		if (spGetInput()->button[SP_BUTTON_L_NOWASD] && spGetInput()->axis[1] < 0)
		{
			spStopKeyboardInput();
			update_options_window(momWindow);
			update_message_window(momWindow);
			transit = 2;
			transit_counter = TRANSIT_TIME;
			return 0;
		}
	}
	if (serverWindow.next)
	{
		if (spGetInput()->button[SP_BUTTON_L_NOWASD] && spGetInput()->axis[0] > 0)
		{
			spStopKeyboardInput();
			//finding next
			goalWindow = momWindow->next;
			if (goalWindow == NULL)
				goalWindow = &serverWindow;
			if (showMessage)
			{
				update_message_window(momWindow);
				update_message_window(goalWindow);
			}
			else
			{
				update_options_window(momWindow);
				update_options_window(goalWindow);			
			}
			transit = 3;
			transit_counter = TRANSIT_TIME;
		}
		if (spGetInput()->button[SP_BUTTON_L_NOWASD] && spGetInput()->axis[0] < 0)
		{
			//finding previous
			goalWindow = &serverWindow;
			while (goalWindow->next && goalWindow->next != momWindow)
				goalWindow = goalWindow->next;
			spStopKeyboardInput();
			if (showMessage)
			{
				update_message_window(momWindow);
				update_message_window(goalWindow);
			}
			else
			{
				update_options_window(momWindow);
				update_options_window(goalWindow);			
			}
			transit = 1;
			transit_counter = TRANSIT_TIME;
		}
	}
	if (showMessage)
	{
		calc_message_window(momWindow,steps);
		update_message_window(momWindow);
	}
	else
	{
		calc_options_window(momWindow,steps);
		update_options_window(momWindow);
	}	
	return 0;
}

void resize(Uint16 w,Uint16 h)
{
	//Setup of the new/resized window
	spSelectRenderTarget(spGetWindowSurface());
  
	spFontShadeButtons(1);
	//Font Loading
	spFontSetShadeColor(BG1_COLOR);
	if (font)
		spFontDelete(font);
	font = spFontLoad(FONT_LOCATION,FONT_SIZE*spGetSizeFactor()>>SP_ACCURACY);
	spFontAdd(font,SP_FONT_GROUP_ASCII,MAIN_COLOR);//whole ASCII
	spFontAddBorder(font,BG1_COLOR);
	spFontMulWidth(font,spFloatToFixed(0.85f));
	if (font_chat)
		spFontDelete(font_chat);
	font_chat = spFontLoad(FONT_LOCATION,FONT_SIZE_CHAT*spGetSizeFactor()>>SP_ACCURACY);
	spFontAdd(font_chat,SP_FONT_GROUP_ASCII,MAIN_COLOR);//whole ASCII
	spFontAddBorder(font_chat,BG1_COLOR);
	spFontMulWidth(font_chat,spFloatToFixed(0.85f));
	font_chat->maxheight = font_chat->maxheight*3/4;
	if (font_big)
		spFontDelete(font_big);
	font_big = spFontLoad(FONT_LOCATION,FONT_SIZE_BIG*spGetSizeFactor()>>SP_ACCURACY);
	spFontAdd(font_big,SP_FONT_GROUP_ASCII,MAIN_COLOR);//whole ASCII
	spFontAddBorder(font_big,BG1_COLOR);
	if (font_small)
		spFontDelete(font_small);
	font_small = spFontLoad(FONT_LOCATION,FONT_SIZE_SMALL*spGetSizeFactor()>>SP_ACCURACY);
	spFontAdd(font_small,SP_FONT_GROUP_ASCII,SEC_COLOR);//whole ASCII
	spFontAddBorder(font_small,BG1_COLOR);
	spFontAddButton( font_small, 'R', SP_BUTTON_START_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); //Return == START
	spFontAddButton( font_small, 'B', SP_BUTTON_SELECT_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); //Backspace == SELECT
	spFontAddButton( font_small, 'l', SP_BUTTON_L_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); // q == L
	spFontAddButton( font_small, 'r', SP_BUTTON_R_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); // e == R
	spFontAddButton( font_small, 'o', SP_PRACTICE_OK_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); //a == left button
	spFontAddButton( font_small, 'c', SP_PRACTICE_CANCEL_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); // d == right button
	spFontAddButton( font_small, '3', SP_PRACTICE_3_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); // w == up button
	spFontAddButton( font_small, '4', SP_PRACTICE_4_NOWASD_NAME, SEC_COLOR, BG2_COLOR ); // s == down button
	spFontAddArrowButton( font_small, '<', SP_BUTTON_ARROW_LEFT, SEC_COLOR, BG2_COLOR );
	spFontAddArrowButton( font_small, '^', SP_BUTTON_ARROW_UP, SEC_COLOR, BG2_COLOR );
	spFontAddArrowButton( font_small, '>', SP_BUTTON_ARROW_RIGHT, SEC_COLOR, BG2_COLOR );
	spFontAddArrowButton( font_small, 'v', SP_BUTTON_ARROW_DOWN, SEC_COLOR, BG2_COLOR );	//spFontMulWidth(font_small,14<<SP_ACCURACY-4);
	spFontMulWidth(font_small,spFloatToFixed(0.85f));

	spSetVirtualKeyboard(SP_VIRTUAL_KEYBOARD_ALWAYS,0,h-w*48/320,w,w*48/320,spLoadSurface("./data/keyboard320.png"),spLoadSurface("./data/keyboardShift320.png"));
}

int main(int argc, char **argv)
{
	spInitCore();
	spSetAffineTextureHack(0); //We don't need it :)
	spInitMath();
	spInitNet();
	screen = spCreateDefaultWindow();
	resize(screen->w,screen->h);
	spSetZSet(0);
	spSetZTest(0);
	
	config = spConfigRead("settings.ini","sissi");

	serverWindow.next = NULL;
	serverWindow.kind = 0;
	
	
	sprintf(serverWindow.data.server.name,"%s",spConfigGetString(config,"server_name","irc.freenode.net"));
	sprintf(serverWindow.data.server.port,"%s",spConfigGetString(config,"server_port","6667"));
	sprintf(serverWindow.data.server.nickname,"%s",spConfigGetString(config,"nickname","Sissiuser"));
	sprintf(serverWindow.data.server.username,"%s",spConfigGetString(config,"username","Sissiuser"));
	sprintf(serverWindow.data.server.realname,"%s",spConfigGetString(config,"realname","Elisabeth Amalie Eugenie"));
	sprintf(oldName,"%s",serverWindow.data.server.name);
	sprintf(oldPort,"%s",serverWindow.data.server.port);
	sprintf(oldNickname,"%s",serverWindow.data.server.nickname);
	sprintf(oldUsername,"%s",serverWindow.data.server.username);
	sprintf(oldRealname,"%s",serverWindow.data.server.realname);
	serverWindow.data.server.selection = 0;
	serverWindow.data.server.server = spNetIRCConnectServer(serverWindow.data.server.name,atoi(serverWindow.data.server.port),serverWindow.data.server.nickname,serverWindow.data.server.username,serverWindow.data.server.realname);
	if (serverWindow.data.server.server)
	{
		serverWindow.first_message = &(serverWindow.data.server.server->first_message);
		serverWindow.last_read_message = &(serverWindow.data.server.server->last_read_message);
	}
	else
	{
		serverWindow.first_message = NULL;
		serverWindow.last_read_message = NULL;
	}
	serverWindow.message_window = spCreateSurface(screen->w,screen->h);
	serverWindow.options_window = spCreateSurface(screen->w,screen->h);
	serverWindow.block = NULL;
	serverWindow.scroll = -1;
	spPollKeyboardInput(serverWindow.data.server.name,256,SP_PRACTICE_OK_NOWASD_MASK);

	spLoop( draw, calc, 10, resize, NULL );
	
	if (serverWindow.data.server.server)
		spNetIRCCloseServer(serverWindow.data.server.server);
	
	spFontDelete(font);
	spFontDelete(font_big);
	spFontDelete(font_small);
	
	pWindow mom = &serverWindow;
	while (mom)
	{
		pWindow next = mom->next;
		spDeleteSurface(mom->message_window);
		spDeleteSurface(mom->options_window);
		if (mom->block)
			spDeleteTextBlock(mom->block);
		if (mom->kind)
			free(mom);
		mom = next;
	}
	
	save_config();
	spQuitNet();
	spQuitCore();
	return 0;
}
