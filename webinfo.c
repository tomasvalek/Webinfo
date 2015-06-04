/*
 * File:	webinfo.c
 * Date:	17.03.2012
 * Author:	Tomas Valek, xvalek02@stud.fit.vutbr.cz
 * Desc:	Webinfo connects via BSD socket to server on port 80 via HTTP 1.1 and
			get required information from HTTP header.
 */	

#include <stdio.h>		//standard input/output library functions
#include <stdlib.h>		//general funciton of language C
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>  	//sscanf
#include <netdb.h>  	//hostent
#include <netinet/in.h>
#include <unistd.h> 	//gethostname
#include <sys/types.h>  //socket
#include <sys/socket.h> //socket
#include <arpa/inet.h>

#define MAX_URL_LENGTH 4096
#define MAX_MESSAGE_LENGTH 256
#define HTTP			7	//strlen of http://
#define MAX_PARAMS 		4
#define PORT			80

//STATE HTTP CODES
char *stateCodes4xx [] = {
	"Error 400 bad request.\n",
	"Error 401 unauthorized.\n",
	"Error 402 paymentRequired.\n", 
	"Error 403 forbidden.\n",
	"Error 404 not found.\n",
	"Error 405 method not allowed.\n",
	"Error 406 not accetable.\n",
	"Error 407 proxy authentication required.\n",
	"Error 408 request timeout.\n",
	"Error 409 conflict.\n",
	"Error 410 gone.\n",
	"Error 411 length required.\n",
	"Error 412 precondition failed.\n",
	"Error 413 request entity is too large.\n",
	"Error 414 request URI too long.\n",
	"Error 415 insupported media type.\n",
	"Error 416 requested range not satisfiable.\n",
	"Error 417 expectation failed.\n",
};

char *stateCodes5xx [] = {
	"Error 500 internal server error.\n",
	"Error 501 not implemented.\n",
	"Error 502 bad gateway.\n",
	"Error 503 service unavailable.\n",
	"Error 504 gateway timeout explained.\n",
	"Error 505 HTTP version not supported.\n",
};

//Params
typedef struct {
	int l_paramOrder;
	int s_paramOrder;
	int m_paramOrder;
	int t_paramOrder;
} Tparameter;

//URL
typedef struct {
	char host[MAX_URL_LENGTH];	//HOST:url.host  				 	www.google.com
	int address;	 			//ADDRESS:(url.host)+url.address 	www.google.com/logo.png
	int port;	   				//PORT:url.port				   		80
} Turl;

//Message
typedef struct{
	char message[MAX_URL_LENGTH];
} Tmessage;

//PROTOTYPES
int main( int argc, char* argv[] );
void checkParam(int argc, char* argv[], Tparameter* param, int* urlPosition);
void checkURL(char* argv[], int urlPosition, Turl* url);
void createMessage(Tmessage* z, Turl url);
int connectTo(Tmessage* z , Turl* url );
void redirectURL(char* argv[], Tmessage z);
int errorHTTP(Tmessage z);
void writeMessage(Tmessage z, Tparameter param);
void findSpecifiedParam(Tmessage z, char header[]);
void closeSocket(int s);

//ERRORS STATES
enum ERROR {
	ERR_PARAM = 0,
	ERR_NO_PARAM,
	ERR_NO_URL,
	ERR_URL,
	ERR_PORT,
	ERR_GETHOSTBYNAME,
	ERR_ERR,
};

char *errorMessages [] = {
	//ERR_PARAM:
	"Params failed! For help -h.\n",
	//ERR_NO_PARAM:
	"Error no param! For help -h.\n",
	//ERR_NO_URL:
	"Error URL not specified! For help -h.\n",
	//ERR_URL:
	"Incorrect URL. Correct URL for example: http://www.google.com. For help -h.\n",
	//ERR_PORT:
	"Incorrect port! For help -h.\n",
	//ERR_GETHOSTBYNAME:
	"Error gethostbyname():",
	//ERR_ERR:
	"Undefined error. For help -h.\n",
};

//HELP:
char HELP[] =
"Webinfo.\n"
"Author: Tomas Valek.\n"
"Date: 05.07.2012.\n"
"Webinfo connects via BSD socket to server on port 80 via HTTP 1.1 and\
get required information from HTTP header.\n"
"Use: ./webinfo [-l][-s][-m][-t] URL\n"
"-l: show object size\n"
"-s: show identifier of server\n"
"-m: show the latest modification\n"
"-t: show type of content\n"
"If require item of header is not available, item show:N/A.\n";

int main( int argc, char* argv[] ){

	Tparameter param;

	Turl url;   	//URL host
	Tmessage z;  	//message from client to server and back

	param.l_paramOrder = -1;
	param.s_paramOrder = -1;
	param.m_paramOrder = -1;
	param.t_paramOrder = -1;

	url.port = PORT;
	url.address = 0;

	int urlPosition = 0;	//url position in argv
	int returnValue = 0;

	checkParam(argc, argv, &param, &urlPosition);

	//usave to url.host form:host[\0port]\0address\0
	checkURL(argv, urlPosition, &url);

	createMessage(&z, url);

	returnValue = connectTo(&z, &url);

	if ( returnValue >= 400 && returnValue <= 417 ) {//state codes 4xx
		returnValue = returnValue-400;
		fprintf(stderr, "%s", stateCodes4xx[returnValue]);
		exit(-1);
	}
	else if ( returnValue >= 500 && returnValue <= 505 ) {//state codes 5xx
		returnValue = returnValue-500;
		fprintf(stderr, "%s", stateCodes5xx[returnValue]);
		exit(-1);
	}

	while ( returnValue == 301 || returnValue == 302 ) {//301 a 302 = redirect
		redirectURL(argv, z);

		checkURL(argv, 0, &url); 

		createMessage(&z, url);

		returnValue = connectTo(&z, &url);

		if ( returnValue >= 400 && returnValue <= 417 ) {
			returnValue = returnValue-400;
			fprintf(stderr, "%s", stateCodes4xx[returnValue]);
			exit(-1);
		} else if ( returnValue >= 500 && returnValue <= 505 ) {
			returnValue = returnValue-500;
			fprintf(stderr, "%s", stateCodes5xx[returnValue]);
			exit(-1);
		}

	}

	writeMessage(z, param);

	return EXIT_SUCCESS;
}
/*******************************************************************************
							   SHOW MESSAGE
*******************************************************************************/
/*Show message to STDOUT by params or all header.*/
void writeMessage(Tmessage z, Tparameter param){

	if ( param.l_paramOrder == -1 && param.s_paramOrder == -1 && param.m_paramOrder == -1 \
		&& param.t_paramOrder == -1 ) {
		printf("%s\n", z.message);
	} else {

		for( int i = 0; i < MAX_PARAMS; i++ ) {

			if ( i == param.l_paramOrder ) {//param_l

				char find[] = "Content-Length:";
				findSpecifiedParam(z, find);

				param.l_paramOrder = -1;
			}

			if ( i == param.s_paramOrder ) {//param_s

				char find[] = "Server:";
				findSpecifiedParam(z, find);

				param.s_paramOrder = -1;
			}

			if ( i == param.m_paramOrder ) {//param_m

				char find[] = "Last-Modified:";
				findSpecifiedParam(z, find);

				param.m_paramOrder = -1;
			}

			if ( i == param.t_paramOrder ) {//param_t

				char find[] = "Content-Type:";
				findSpecifiedParam(z, find);

				param.t_paramOrder = -1;
			}
		}
	}   
}

/*******************************************************************************
					   FIND USER SPECIFIED PARAM IN HEADER
*******************************************************************************/
void findSpecifiedParam(Tmessage z, char header[]){

	char* tmp = NULL;
	char na[] = "N/A";

	if( (tmp = (strstr(z.message, header))) != NULL ) {//if exists

		char tmpArray[MAX_MESSAGE_LENGTH];	//tmp array for write
		int j = 0;

		while (1) {
			if( tmp[j] == '\r' || tmp[j] == '\n' )
				break;

			tmpArray[j] = tmp[j];
			j++;
		}

		tmpArray[j] = '\0';
		printf("%s\n", tmpArray);
	} else //not exists
		printf("%s\n", strcat(header, na));

}

/*******************************************************************************
								 ERROR HTTP
*******************************************************************************/

/*Get error number from HTTP.*/
int errorHTTP(Tmessage z) {

	int i = 0;
	int returnValue = 0;
	int returnValueHTTP = 0;

	char tmp[MAX_URL_LENGTH];

	while ( z.message[i] != '\n' ) {
		tmp[i] = z.message[i+9]; //9 == strlen(HTTP/1.1)+one space
		i++;
	}

	for(int j = 0; j < 3; j++){ //three digit number
		tmp[j] = tmp[j];
		i++;
	}

	tmp[3] = '\0';

	returnValue = sscanf(tmp, "%d", &returnValueHTTP);

	//kontrola sscanf
	if ( returnValue == 1 ){
		//OK
	} else if ( errno != 0 ) {
		perror("scanf");
		exit(-1);
	} else {
		fprintf(stderr, "%s", errorMessages[ERR_ERR]);
		exit(-1);
	}

	return returnValueHTTP;
}
/*******************************************************************************
								REDIRECT URL
*******************************************************************************/
/*Save to argv new URL, from Location:*/
void redirectURL(char* argv[], Tmessage z) {

	char* tmp = strstr(z.message,"Location: ");
	int i = 0;

	while( tmp[10+i] != '\r' ) {
		argv[0][i] = tmp[10+i];
		i++;
	}

	argv[0][i] = '\0';
}
/*******************************************************************************
								   CONNECT
*******************************************************************************/
/*Create connect -> send request from z->message and save result to z->message.
If redirect -> return 1.
*/
int connectTo(Tmessage* z , Turl* url ) {

	struct hostent* host;		//remote computer
	struct sockaddr_in s_in;	//internet address

	int s;  				//socket
	int n;  				//read
	int returnValue = 0;	//return value from errorHTTP();

	/*create socket
		AF_INET
		SOCK_STREAM	 -> TCP
		IPPROTO_TCP	 -> TCP
	*/
	if ( (s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) { //socket fail
		perror("Error pri vytvareni socketu:");
		exit(-1);
	}

	s_in.sin_family = AF_INET;			//type of address
	s_in.sin_port = htons(url->port);  //port

	if ( (host = gethostbyname(url->host)) == NULL ){ //gethostname fail
		closeSocket(s);
		fprintf(stderr, "%s %s", errorMessages[ERR_GETHOSTBYNAME], url->host);
		exit(-1);
	}

	//settings to host IP address
	memcpy( &s_in.sin_addr, host->h_addr_list[0], host->h_length);

	/*connect
		s						 	ID socket
		(struct sockaddr *)&s_in	address TCP
		sizeof(s_in				 	size
	*/
	if (connect (s, (struct sockaddr *)&s_in, sizeof(s_in) ) < 0 ){//connect fail
		closeSocket(s);
		perror("Error pripojeni!");
		exit(-1);
	}

	/*send data to server
	   s	ID socket
	*/
	if ( write(s, z->message, strlen(z->message)) < 0 ){ //write fail
		closeSocket(s);
		perror("Error pri odesilani dat na server:");
		exit(-1);
	}

	/*read data from server
		s				ID socket
		buffer  		receiving data can be interupted, pointer to z->message
		buffer_size	 	buffer size
	*/
	char* buffer = z->message;
	int buffer_size = sizeof(z->message);

	do {/*wauiting for read, for all data or buffer will be full*/
		if ( ( n = read(s, buffer, buffer_size) ) < 0 ) {//read fail
			closeSocket(s);
			perror("Error pri prijimani dat:");
			exit(-1);
		}

		buffer = buffer + n;
		buffer_size = buffer_size-n; //check buffer size

	} while( n != 0 && buffer_size != 0);

	*buffer = '\0'; //when server return data without '\0'

	closeSocket(s);

	if ( (strstr(z->message,"200 OK")) == NULL ||
		 (strstr(z->message,"302 Found" )) == NULL ||
		 (strstr(z->message,"301 Moved Permanently")) == NULL ) {
		returnValue = errorHTTP(*z);
		return returnValue;
	}

	return 0;
}

void closeSocket(int s) {
	if ( close(s) < 0 )	{
		perror("Close socket error:");
		exit(-1);
	}
}

/*******************************************************************************
								CREATE MESSAGE
*******************************************************************************/
/*Create message to form:
"HEAD / HTTP/1.1\r\nHost:www.google.com\r\nConnection: close\r\n\r\n"*/
void createMessage(Tmessage* z, Turl url) {

	z->message[0] = '\0'; //init

	strcpy(z->message,"HEAD /");

	if ( url.address != 0 )	{
		strcat(z->message,(url.host)+url.address);
	}

	strcat(z->message," HTTP/1.1\r\nHost: ");

	strcat(z->message,url.host);

	strcat(z->message,"\r\nConnection: close\r\n\r\n");
}
/*******************************************************************************
								 CHECK URL
*******************************************************************************/
/*Remove:"http://" Find ':' and replace by '\0' then '/' replace by '\0' and save
to url->address position where was address. To url->port save port.
We have 3 items in array in form:host[\0port]\0address\0
*/
void checkURL(char* argv[], int urlPosition, Turl* url) {

	int strLength = 0;
	int portPosition = 0;
	int returnValue = 0;
	bool port = false;

	strLength = strlen(argv[urlPosition]);

	//if exist remove "http://"
	if ( (strstr(argv[urlPosition], "http://")) != NULL ) {
		//new strlen
		strLength = strLength-HTTP;
		//remove http://
		for ( int i = 0; i < strLength; i++ )
			url->host[i] = argv[urlPosition][i+HTTP];

		url->host[strLength] = '\0';
	} else {//"http://" not exist
		fprintf(stderr, "%s", errorMessages[ERR_URL]);
		exit(-1);
	}

	//replace ':' by '\0' and first '/' by '\0'
	for ( int i = 0; i < strLength; i++ ){
		if ( url->host[i] == ':' ) {
			port = true;
			url->host[i] = '\0';
			portPosition = i+1;
		} else if ( url->host[i] == '/' ) {//we find '/' replace by \0 and break
			 url->host[i] = '\0';
			 if ( url->host[i+1] != '\0' ) {
				//there are must not '\0' otherwise write:www.google.com/
				url->address = i+1; //+1 because address starts after '\0'
			 }
			break;
		}
	}

	if ( port )	{
		//convert string to number and save to url->port
		errno = 0;
		returnValue = sscanf((url->host)+portPosition, "%d", &(url->port));

		//kontrola sscanf
		if ( returnValue == 1 ) {
			//ok
		} else if ( errno != 0 ) {
			perror("scanf");
			exit(-1);
		} else {
			fprintf(stderr, "%s", errorMessages[ERR_PORT]);
			exit(-1);
		}
	}
}

/*******************************************************************************
							   CHECK PARAMS
*******************************************************************************/
/*Check params and save order.*/
void checkParam(int argc, char* argv[], Tparameter* param, int* urlPosition) {

	int c;
	int params_lsmt = 0;

	if ( argc == 1 ) {//no params
		fprintf(stderr, "%s", errorMessages[ERR_NO_PARAM]);
		exit(-1);
	} else if ( argc == 2 && ((strcmp("-h", argv[1])) == 0) ) {//help
		printf("%s", HELP);
		exit(0);
	}

	while ( (c = getopt(argc, argv, "lsmt" )) != -1 ) {

		switch(c) {
			case 'l':
				param->l_paramOrder = params_lsmt;
				break;
			case 's':
				param->s_paramOrder = params_lsmt;
				break;
			case 'm':
				param->m_paramOrder = params_lsmt;
				break;
			case 't':
				param->t_paramOrder = params_lsmt;
				break;
			case '?':
				fprintf(stderr, "%s", errorMessages[ERR_PARAM]);
				exit(1);
		}
		params_lsmt++;
	}

	if ( argc == optind ) { //NO URL
		fprintf(stderr, "%s", errorMessages[ERR_NO_URL]);
		exit(-1);
	} else if ( argc != optind+1 || params_lsmt > MAX_PARAMS ) {
	// argc must be < 1, then optind, otherwise any param is multiple enter
		fprintf(stderr, "%s", errorMessages[ERR_PARAM]);
		exit(-1);
	}

	*urlPosition = optind;
}
