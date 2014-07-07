#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>


enum connectionState
{
	STATE_SETUP = 0,
	STATE_WELCOMEMSG,
	STATE_LOGIN,
	STATE_SELECTSERVER,
	STATE_GETLIST,
	STATE_DISCONNECT
};

volatile uint8_t currentState = STATE_SETUP;
char* password;

char *replace_str(const char *str, const char *old, const char *new);

size_t data_receive( char *ptr, size_t size, size_t nmemb, void *userdata)
{
	switch (currentState)
	{
		case STATE_WELCOMEMSG:
		{
			if (strstr(ptr, "Welcome") != NULL)
			{
				//got the welcome message
				currentState = STATE_LOGIN;
			}
			break;
		}
		
		case STATE_GETLIST:
		{
			char buffer[size*nmemb+1];
			strncpy(buffer, ptr, size*nmemb);
			
			char* token = strtok(buffer, "= ");
			bool hasClients = false;
			while (token)
			{
				if (strstr(token, "client_nickname") != NULL)
				{
					//found the client nickname
					char* nickName = strtok(NULL, "= ");
					
					//get the client Type - do not print out client type 1
					strtok(NULL, "= ");
					char* clientType = strtok(NULL, "= ");
					if (clientType[0] != '1')
					{
						printf("%s\n", replace_str(nickName, "\\s", " "));
						hasClients = true;
						/*
						char* nickNameWord = strtok(nickName, "//s");
						while (nickNameWord)
						{
							printf("%s", nickNameWord);
							nickNameWord = strtok(NULL, "//s");
						}
						 */
					}
				}
				token = strtok(NULL, "= ");
			}
			if (!hasClients)
			{
				printf("There are no clients on the server...\n");
			}
			
			currentState = STATE_DISCONNECT;
			break;
		}
		
		default:
		{
			if (strstr(ptr, "error id=0") != NULL)
			{
				//no error - proceed to next state
				currentState++;
			}
			else
			{
				printf("Could not retrieve client list. Check your password.\n");
				currentState = STATE_DISCONNECT;
			}
		}
	}
	return size*nmemb;
}

size_t data_send( void *ptr, size_t size, size_t nmemb, void *userdata)
{
	char toSend[80];
	switch (currentState)
	{
		case STATE_LOGIN:
		{
			strcpy(toSend, "login admin ");
			strcat(toSend, password);
			break;
		}
		
		case STATE_SELECTSERVER:
		{
			strcpy(toSend, "use 1");
			break;
		}
		
		case STATE_GETLIST:
		{
			strcpy(toSend, "clientlist");
			break;
		}
		
		case STATE_DISCONNECT:
		{
			strcpy(toSend, "quit");
			break;
		}
		
		default:
		{
			return 0;
		}
	}
	strcat(toSend, "\r\n");
	strcpy(ptr, toSend);
	return strlen(toSend);
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("\nLists all users on a Teamspeak server using the telnet interface of the server. Logs in with user \"admin\".\nUsage: -p yourPassword hostname:port\n\n");
		return 0;		
	}
		 
	CURL *curl;
	password = argv[argc-2];
	curl = curl_easy_init();
	
	char ebuf[CURL_ERROR_SIZE] = "";
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, ebuf);

	
	char url[256] = "telnet://";
	strcat(url,argv[argc-1]);
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_receive);
	currentState = STATE_WELCOMEMSG;
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, data_send); 

	curl_easy_perform(curl); 
	curl_easy_cleanup(curl);
	
	printf("%s\n", ebuf);


	return 0;
}

//credit:
//http://creativeandcritical.net/str-replace-c/
char *replace_str(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);

	if (oldlen != newlen) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
			count++;
		/* this is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	for (r = ret, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen) {
		/* this is undefined if q - p > PTRDIFF_MAX */
		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, new, newlen);
		r += newlen;
	}
	strcpy(r, p);

	return ret;
}
