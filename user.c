#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#define GNUMBER 58028

void (*old_handler)(int);
void stringToWords(char* input);
void processList(char* input);
void processRequest(char* input);
void processRequest_2(char* buffer);
void processSubmit(char* input);
void cleanNewLine(char* str);


char* words[100];

int ECPport, TESport, nParts=0, nrOptions=0, questSize, varSize=0, filled=0, successList=0, successRequest=0, successRequest2=0;
char ECPname[100], TESname[100], QIDstr[30], deadline[100], buffer_aux[200], questSizeStr[40];

FILE * file;
int fd, sumWrite=0, nRead=0, nWrite=0;




int main(int argc, char *argv[])
{
	int i=0, addrlen, SID, topicNum=0, nRead;
	char SIDstr[20], req_1[20], req_2[20], submit[80];
	struct hostent *hostptr;
	struct sockaddr_in serveraddr;
	char* buffer=(char*)malloc(sizeof(char)*200);
	char* cmd=(char*)malloc(sizeof(char)*500);
	
	
	
	if (argc==6 && !strcmp(argv[2],"-n") && !strcmp(argv[4],"-p"))
	{
		SID=atoi(argv[1]);		
		
		if(SID==0)
		{
			printf("Invalid SID. Program aborted\n");
			exit(-1);
		}
		
		strcpy(SIDstr,argv[1]);	
		strcpy(ECPname,argv[3]);
		ECPport=atoi(argv[5]);
	}
	
	else if(argc==4)
	{
		SID=atoi(argv[1]);
		
		if(SID==0)
		{
			printf("Invalid SID. Program aborted\n");
			exit(-1);
		}
		
		strcpy(SIDstr,argv[1]);

		if(!strcmp(argv[2],"-n"))
		{
			strcpy(ECPname,argv[3]);
			ECPport=GNUMBER;
		}
		
		else if(!strcmp(argv[2],"-p"))
		{
			strcpy(ECPname, "127.0.0.1");
			ECPport=atoi(argv[3]);
		}
		
		else
		{
			printf("Invalit set of arguments. Program aborted\n");
			exit(-1);
		}
	}

	else if(argc==2)
	{
		SID=atoi(argv[1]);
		
		if(SID==0)
		{
			printf("Invalid SID. Program aborted\n");
			exit(-1);
		}
		
		strcpy(SIDstr,argv[1]);
		
		strcpy(ECPname, "127.0.0.1");
		ECPport=GNUMBER;
	}
	
	else
	{
		printf("Invalid set of arguments. Program aborted\n");
		exit(-1);
	}
	
	
	read(0,cmd,500);
	cleanNewLine(cmd);
	stringToWords(cmd);
	
	
	while(1) 
	{	
		
		if(!strcmp(words[0],"list"))
		{			
			if(nParts != 1)
			{
				printf("Incorrect use of list command\n");
				exit(-1);
			}
			
						
			fd=socket(AF_INET,SOCK_DGRAM,0);
			hostptr = gethostbyname(ECPname); 
			memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_addr.s_addr =((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
			serveraddr.sin_port = htons((u_short)ECPport);
		
			addrlen=sizeof(serveraddr);
			sendto(fd, "TQR\n", strlen("TQR\n")+1,0,(struct sockaddr*) &serveraddr, addrlen);		
			addrlen=sizeof(serveraddr);
			nRead=recvfrom(fd, buffer, 200, 0, (struct sockaddr*)&serveraddr, (socklen_t *)&addrlen);
			
			if(buffer[nRead-1]=='\n')
				buffer[nRead-1]='\0';
			else
			{
				printf("ECP message does not end with newline character\n");
				exit(-1);
			}
			
			close(fd);
			
			processList(buffer);
		}	
		
		
		else if(!strcmp(words[0],"request"))
		{	
			if(nParts != 2)
			{
				printf("Incorrect use of request command\n");
				exit(-1);
			}
			
			if(successList==0)
			{
				printf("Topics haven't been listed yet\n");
				exit(-1);
			}
			
			i=atoi(words[1]);
			bzero(buffer_aux, 200);
			sprintf(buffer_aux, "%d", i); 
			if(strlen(words[1])!=strlen(buffer_aux))
			{
				printf("Incorrect use of request command\n");
				bzero(buffer_aux, 200);
				exit(-1);
			}
			bzero(buffer_aux, 200);
			i=0;

			
			topicNum=atoi(words[1]);

			if(topicNum>0 && topicNum<=nrOptions)
			{
				strcpy(req_1,"TER ");
				strcat(req_1,words[1]);
				strcat(req_1,"\n");
		
				fd=socket(AF_INET,SOCK_DGRAM,0);
				hostptr = gethostbyname(ECPname); 
				memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_addr.s_addr =((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
				serveraddr.sin_port = htons((u_short)ECPport);
			
				addrlen=sizeof(serveraddr);
				sendto(fd, req_1, strlen(req_1)+1,0,(struct sockaddr*) &serveraddr, addrlen);		
				addrlen=sizeof(serveraddr);
				nRead=recvfrom(fd, buffer, 200, 0, (struct sockaddr*)&serveraddr, (socklen_t *)&addrlen);
				
				if(buffer[nRead-1]=='\n')
					buffer[nRead-1]='\0';
				else
				{
					printf("ECP message does not end with newline character\n");
					exit(-1);
				}
							
				close(fd);
			
				processRequest(buffer);
				
				
				if(successRequest==0)
				{
					printf("There was an error with the message from ECP\n");
					exit(-1);
				}
				
				
				
				fd=socket(AF_INET,SOCK_STREAM,0);
				hostptr = gethostbyname(TESname); 
				memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_addr.s_addr =((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
				serveraddr.sin_port = htons((u_short)TESport);
				connect(fd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
				
				strcpy(req_2, "RQT ");
				strcat(req_2, argv[1]); // em argv[1] esta o SID
				strcat(req_2, "\n");
				
				write(fd, req_2, 10);
				
				processRequest_2(buffer);
				
			}
			
			
			else
			{
				printf("Invalid topic number\n");
				exit(-1);
			}
				
		}
		
		
		else if(!strcmp(words[0],"submit")) 
		{	
			if(nParts != 6)
			{
				printf("Incorrect use of submit command\n");
				exit(-1);
			}
			
			if(successRequest2==0)
			{
				printf("You haven't requested any topics yet or you have already submited your answer\n");
				exit(-1);
			}
			
			strcpy(submit, "RQS ");
			strcat(submit, SIDstr);
			strcat(submit, " ");
			strcat(submit, QIDstr);
			
			for(i=0;i<5;i++)
			{
				strcat(submit, " ");
				strcat(submit, words[i+1]);
			}
			
			strcat(submit, "\n");

			
			
			
			fd=socket(AF_INET,SOCK_STREAM,0);
			hostptr = gethostbyname(TESname); 
			memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_addr.s_addr =((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
			serveraddr.sin_port = htons((u_short)TESport);
			connect(fd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
			
			write(fd, submit, 80);
			nRead=read(fd, buffer, 200);
			
			
			if(buffer[nRead-1]=='\n')
			{
				buffer[nRead-1]='\0';
			}
			
			else
			{
				printf("ECP message does not end with newline character\n");
				exit(-1);
			}

			close(fd);						
			
			processSubmit(buffer);		
			
		}
		
		
		else if(!strcmp(words[0],"exit"))
		{			
			if(nParts != 1)
			{
				printf("Incorrect use of exit command\n");
				exit(-1);
			}
			
			free(cmd);
			for(i=0;i<nParts;i++)
				free(words[i]);
			free(buffer);
			
			printf("Program exited successfully\n");
			
			return 0;
		}
		
		else
		{
			printf("Unknown command\n");
		}
			
		bzero(cmd, 500);
		read(0, cmd, 500);
		cleanNewLine(cmd);
		stringToWords(cmd);
	}
	
	
	free(cmd);
	for(i=0;i<nParts;i++)
		free(words[i]);
	free(buffer);
	
	close(fd);
	
	return 0;
}





void stringToWords(char* input)
{
	int i=0;
	const char s[2] = " ";
	char *token;
	
	if(filled==0)
		filled=1;
	
	else
	{
		for(i=0;i<nParts;i++)
			free(words[i]);
	}
	
	i=0;
	nParts=0;
	token = strtok(input, s);
	
	while(token!=NULL) 
	{
		words[i]=(char*)malloc(sizeof(char)*(strlen(token)+1));
		strcpy(words[i],token);
		i++;
		nParts++;
		token = strtok(NULL, s);
		
	}
	
}



void processList(char* input)
{
	int i=0;
	char newInput[200];
	
	strcpy(newInput,input);
	stringToWords(newInput);

	
	if(!strcmp(words[0],"EOF"))
	{
		printf("TQR request cannot be answered. Perhaps there are no questionanire topics available\n");
	}

	else if(!strcmp(words[0],"ERR"))
	{
		printf("TQR request was not correctly formulated\n");
	}

	else if(!strcmp(words[0],"AWT"))
	{
		if(nParts < 3)
		{
			printf("Invalid message from ECP\n");
			return;
		}
			
		nrOptions=atoi(words[1]);
		
		if(nrOptions==0)
		{
			printf("Second argument isn't a number\n");
			return;
		}
		
		if(nrOptions>99)
		{
			printf("Topics limit exceeded\n");
			return;
		}
		
		if(nParts != (nrOptions+2))
		{
			printf("Number of questionnaire topics different than stated\n");
			return;
		}
		
		i=atoi(words[1]);
		bzero(buffer_aux, 200);
		sprintf(buffer_aux, "%d", i); 
		if(strlen(words[1])!=strlen(buffer_aux))
		{
			printf("Second argument isn't a number\n");
			bzero(buffer_aux, 200);
			return;
		}
		bzero(buffer_aux, 200);
		i=0;
		
				
		for(i=0;i<nrOptions;i++)
		{
			printf("%d- %s\n",i+1,words[i+2]);
		}
		
		successList=1;
	}

	else
	{
		printf("Invalid message from ECP\n");
	}		
		
}


void processRequest(char* input)
{
	char newInput[200];
	
	strcpy(newInput,input);
	stringToWords(newInput);
	
	successRequest=0;
	
	
	if(!strcmp(words[0],"EOF"))
	{
		printf("TER request cannot be answered. Perhaps the topic number is invalid\n");
	}

	else if(!strcmp(words[0],"ERR"))
	{
		printf("TER request was not correctly formulated\n");
	}
	
	else if(!strcmp(words[0],"AWTES"))
	{
		if(nParts != 3)
		{
			printf("Invalid message from ECP\n");
			return;
		}
		
		strcpy(TESname, words[1]);
		TESport=atoi(words[2]);
		
		if(TESport==0)
		{
			printf("Invalid TES IP\n");
			return;
		}

		
		bzero(buffer_aux, 200);
		sprintf(buffer_aux, "%d", TESport); 
		if(strlen(words[2])!=strlen(buffer_aux))
		{
			printf("Invalid message from ECP\n");
			bzero(buffer_aux, 200);
			return;
		}
		bzero(buffer_aux, 200);		
		
		printf("TES IP: %s\nTES Port: %s\n", words[1], words[2]);
		
		successRequest=1;
	}
	
	else
	{
		printf("Invalid message from ECP\n");
	}
	
}


void processRequest_2(char* buffer)
{
	int i=0, sumRead=0;
	
	nRead=read(fd, buffer, 4);				
				
	if(buffer[3]==' ')
	{
		buffer[3]='\0';
			
		if(strcmp(buffer,"AQT"))
		{
			printf("Invalid message from TES\n");
			exit(-1);
		}
	}
	
	else if(buffer[3]=='\n')
	{
		buffer[3]='\0';
			
		if(!strcmp(buffer,"ERR"))
		{
			printf("Request was not well formulated\n");
			return;
		}
	}
	
	else
	{
		printf("Invalid message from TES\n");
		exit(-1);
	}
	
	bzero(buffer, 200);
	bzero(buffer_aux, 200);		
	varSize=0;
	
	while(1)
	{
		nRead=read(fd, buffer, 1);

		if(buffer[0]=='\n')
		{
			printf("Invalid message from TES\n");
			exit(-1);
		}
		
		if(buffer[0]==' ')
			break;
		
		buffer_aux[varSize]=buffer[0];
							
		if(varSize==24)
		{
			printf("Invalid message from TES\n");
			exit(-1);
		}
		
		varSize++;					
	}
	
	if(varSize==0)
	{
		printf("Invalid message from TES\n");
		exit(-1);
	}
	
	
	buffer_aux[varSize]='\0';
	strcpy(QIDstr, buffer_aux);
	
	bzero(buffer, 200);
	bzero(buffer_aux, 200);
	
	nRead=read(fd, buffer, 19); // reading time and following space
	
	if(buffer[nRead-1]!=' ')
	{
		printf("Invalid message from TES\n");
		exit(-1);
	}
	
	for(i=0;i<18;i++) // testing if there is no space in between
	{
		if(buffer[i]==' ')
		{
			printf("Invalid message from TES\n");
			exit(-1);
		}
	}
	
	i=0;
	varSize=0;
	bzero(buffer, 200);
	bzero(buffer_aux, 200);
	
	while(1)
	{
		nRead=read(fd, buffer, 1);

		if(buffer[0]=='\n')
		{
			printf("Invalid message from TES\n");
			exit(-1);
		}
		
		if(buffer[0]==' ')
			break;
		
		buffer_aux[varSize]=buffer[0];
		
		varSize++;					
	}
	
	if(varSize==0)
	{
		printf("Invalid message from TES\n");
		exit(-1);
	}
	
	buffer_aux[varSize]='\0';
	strcpy(questSizeStr, buffer_aux);
	
	questSize=atoi(questSizeStr);				
	if(questSize==0)
	{
		printf("Invalid questionnaire size\n");
		exit(-1);
		
	}
	
	varSize=0;
	bzero(buffer, 200);
	bzero(buffer_aux, 200);
	
	sprintf(buffer_aux, "%d", questSize); 
	if(strlen(questSizeStr)!=strlen(buffer_aux))
	{
		printf("Invalid questionnaire size\n");
		exit(-1);
	}
					
	
	if((file = fopen("Computer_Networking-Questionnaire.pdf", "wb"))==NULL)
	{
		printf("Error opening file\n");
		exit(-1);
	}
	
	sumWrite=0;
	sumRead=0;
	bzero(buffer, 200);
	bzero(buffer_aux, 200);
	
	while(sumWrite<questSize)
	{
		if(sumRead<questSize){
			nRead=read(fd,buffer,200);
			sumRead+=nRead;
			
			if(nRead<0)
			{	
				printf("%d\n", sumRead);
				printf("Error reading answer from TES\n");
				exit(-1);
			}

			if(nRead==0)
			{
				perror("Error: nothing to read\n");
				exit(-1);
			}
		}
		
		if(sumRead>questSize+1){
			printf("Questionnaire too long\n");
			exit(-1);
		}
		
		else if(sumRead==questSize+1)
		{	
			if(buffer[nRead-1]!='\n')
			{
				printf("Missing newLine character\n");
				exit(-1);
			}
			
			else if(buffer[nRead-1]=='\n')
			{
				buffer[nRead-1]='\0';
				nRead--;
			}
		}
		
		nWrite=fwrite(buffer, 1, nRead, file);

		if(nWrite<0)
		{
			printf("ERROR: writing on file\n");
			exit(-1);
		}

		if(nWrite==0)
		{
			printf("ERROR: nothing to write\n");
			exit(-1);
		}

		sumWrite+=nWrite;
	
		bzero(buffer, 200);

	}
					
	printf("DONE SENDING\n");
	printf("Received file %s\n", QIDstr);
	nRead=fclose(file);
			
	bzero(buffer, 200);
	
	successRequest2=1; // sera que e aqui que activo esta flag?

}



void processSubmit(char* input)
{
	char newInput[200];
	char scoreStr[5];
	int score;
	
	strcpy(newInput,input);
	stringToWords(newInput);
	
	
	if(!strcmp(words[0],"ERR"))
	{
		printf("Request was not correctly formulated\n");
	}
	
	else if(!strcmp(words[0],"AQS"))
	{
		if(nParts != 3)
		{
			printf("Invalid message from TES\n");
			return;
		}
		 
		score=atoi(words[2]);
		strcpy(scoreStr,words[2]);
		strcat(scoreStr, "%");
		
		if(score>=0 && score<=100)
			printf("Obtained score: %s\n", scoreStr);
		
		else if(!strcmp(words[2],"-1"))
			printf("Questionnaire submited after deadline\n");
			
		else if(!strcmp(words[2],"-2"))
			printf("SID-QID pair does not match\n");
		
		successRequest2=0; //sera que devo por aqui??
	}
	
	else
	{
		printf("Invalid message from TES\n");
	}
	
}





void cleanNewLine(char* str)
{
	int len=strlen(str);
	if(str[len-1]=='\n')
		str[len-1]='\0';
		
	if(strlen(str)==0)
	{
		printf("Invalid command\n");
		exit(-1);
	}
}





