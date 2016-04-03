#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#define ARGMAX 9
#define ARGMIN 3

extern int errno;

int verifyDeadline(char* timer);

static char months[12][10] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

int main(int argc, char** argv){

int i, fd, newfd,fd2,addrlen,n,tesport=59000,ecpport=58028,sumread=0;
char stesport[6],secpport[6],ecpname[50];
struct hostent *hostptr;
struct sockaddr_in serveraddr, clientaddr;
int clientlen,sent=0;
char buffer[100], *token,filename[13],answer[100],databuffer[200], letters[6],letter[2];
char aux[200];
int num,intlen,bufflen;
const char s[2]=" ";
const char t[2]="\n";
int find=0;


const char topicname[26];
int topicnumber=1, numberofq = 4;


int val,val2,sid;
FILE* file;
long fsize;
int nwrite=0,nread=0,sumwrite=0;
char qid[25], timer[19];
pid_t pid;
void (*old_handler_child)(int);
void (*old_handler_pipe)(int);

if((old_handler_child=signal(SIGCHLD,SIG_IGN))==SIG_ERR){
    perror("SIG_ERR child\n");
    exit(-1);
}

if((old_handler_pipe=signal(SIGPIPE,SIG_IGN))==SIG_ERR){
    perror("SIG_ERR pipe\n");
    exit(-1);
}

strcpy(ecpname,"127.0.0.1");

//PROTEGER MELHOR
if(argc>ARGMIN && argc<ARGMAX+1){
    i=ARGMIN;
    while(i<argc){
	if(!strcmp(argv[i],"-p")){
	    strcpy(stesport,argv[i+1]);
	    tesport=atoi(stesport);
	}
	
	else if(!strcmp(argv[i], "-n")){
	    strcpy(ecpname,argv[i+1]);
	    printf("%s\n",ecpname);
	}
	
	else if(!strcmp(argv[i], "-e")){
	    strcpy(secpport,argv[i+1]);
	    ecpport=atoi(secpport);
	}
    i=i+2;
    }
}
else if(argc<ARGMIN){
  printf("ERROR: too few arguments\n");
  exit(-1);
}

else if(argc>ARGMAX)
  printf("ERROR: too many arguments\n");

strcpy(topicname,argv[1]);
topicnumber=atoi(argv[2]);
printf("%d\n", topicnumber);

bzero(buffer, sizeof(buffer));
bzero(aux, sizeof(aux));


//Abertura do socket para ligacao TCP
if((fd=socket(AF_INET,SOCK_STREAM,0))==-1){
    printf("ERROR: opening socket");
    exit(-1);
}

//Associacao de ip e port ao socket 
memset((void*)&serveraddr,(int)'\0',sizeof(serveraddr));
serveraddr.sin_family=AF_INET;
serveraddr.sin_addr.s_addr = htonl (INADDR_ANY);
serveraddr.sin_port = htons ((u_short)tesport);

//Ligacao do socket do cliente
if((bind(fd,(struct sockaddr*)&serveraddr, sizeof(serveraddr)))==-1){
    perror("ERROR: on bind\n");
    exit(-1);
}
if(listen(fd, 5)==-1){
    printf("ERROR: on listen\n");
    exit(-1);
}

clientlen=sizeof(clientaddr);

sprintf(buffer, "%s%02d%s","T", topicnumber,"quest.txt");
file=fopen(buffer,"w+");
if(file==NULL){
  printf("Error: creating file\n");
  exit(-1);
}

fclose(file);


while(1){
  if((newfd = accept (fd,(struct sockaddr*)&clientaddr, &clientlen))==-1){
    printf("ERROR: on accept\n");
    exit(-1);
  }
  while(newfd==-1 && errno==EINTR);//fork not affected by interruptions

  if(newfd==-1)
    exit(-1);

  if((pid=fork())==-1)
    exit(-1);

  else if(pid==0){
      bzero(buffer, sizeof(buffer));
      bzero(aux, sizeof(aux));

      nread=read(newfd, buffer,45);
//       printf("READ %d\n", nread);

      if(nread==0){
	printf("ERROR: nothing to read\n");
	exit(-1);
      }

      else if(nread<0){
	perror("ERROR: reading request\n");
	write(newfd, "ERR\n", 4);
	exit(-1);
      }

      if(buffer[nread-1]!='\n'){
	      printf("ERROR: missing a new line\n");
	      write(newfd, "ERR\n", 4);
	      exit(-1);
      }


      strtok(buffer,t);

      token = strtok(buffer, s);
      // printf("AINDA\n");
      if(!strcmp(buffer,"RQT")){
      /*  
      printf("ENTREI\n");*/
	    bzero(answer, sizeof(answer));
	    token = strtok(NULL,s);	
	    
	    sid=atoi(token);
	    sprintf(aux,"%d",sid);
	    

	    intlen=strlen(aux);
	    bufflen=strlen(token);
	    
	    //Verifica se o SID e valido
	    if(intlen!=5 || intlen!=bufflen){
		printf("ERROR: invalid SID\n");
		write(newfd, "ERR\n", 4);
		exit(-1);
	    }
	    
	    token=strtok(NULL,s);
      
	    //Verifica se nao ha mais argumentos
	    if(token!=NULL){
		  printf("ERROR: too many arguments in RQT\n");
		  write(newfd, "ERR\n", 4);
		  exit(-1);
	    }
	    
	    //Se satisfizer tudo, entao responde com AQT
	    else{
	      printf("Request: %s %s %d\n", aux, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	      //QID TEM DE SER UNICO
	      strcpy(answer,"AQT ");
	      strcpy(qid, aux);

	      //DATA TEM DE SER GERADA
	      time_t tim = time(NULL);
	      struct tm tm = *localtime(&tim);
	      sprintf(timer,"%02d%s%d%s%02d%s%02d%s%02d" ,tm.tm_mday+2,months[tm.tm_mon],tm.tm_year+1900,"_",14,":",00,":",00);
	      sprintf(aux,"%02d%s%d%s%02d%s%02d%s%02d" ,tm.tm_mday,months[tm.tm_mon],tm.tm_year+1900,"_",tm.tm_hour,":",tm.tm_min,":",tm.tm_sec);

	      
	      strcat(qid, "_");
	      strcat(qid,aux);
	      bzero(databuffer, sizeof(databuffer));
	      sprintf(databuffer,"%s%s%d%s%s%s" ,qid, " ",sid ," ",timer," ");
	      strcat(answer,qid);
	      strcat(answer," ");
	      strcat(answer,timer);
	      strcat(answer, " ");
	      
	      
	      //Escolhe um ficheiro dos questionarios
	      int x = (rand() % numberofq)+1;
	      sprintf(filename,"%s%02d%s%03d%s","T",topicnumber,"QF",x,".pdf");
	      strcat(databuffer,filename);
	      strcat(databuffer," \n");
	      sprintf(buffer, "%s%02d%s","T", topicnumber,"quest.txt");
	      file=fopen(buffer,"a");
	      
	      if(file==NULL){
		printf("Error: opening file\n");
		exit(-1);
	      }
	      
	      fwrite(databuffer, 1,64,file);

	      fclose(file);
	      

	      printf("FILE %s\n",filename);
	      file=fopen(filename,"rb");

	      if (file == NULL) 
	      {
		  printf("File not found!\n");
		  exit(-1);
	      }
	      
	      else 
	      {

		  fseek(file, 0, SEEK_END);
		  fsize = ftell(file);
		  rewind(file);
		  
		  sprintf(aux,"%ld", fsize);
		  strcat(aux," ");
		  strcat(answer,aux);

	      }
	      
	      write(newfd, answer,strlen(answer));
	      
	      printf("ANSWER %s %d\n", answer, (int)strlen(answer));
	      bzero(buffer,sizeof(buffer));
	      sumread=0;
	      sumwrite=0;
	      while(!feof(file)) {
		  nread=fread(buffer, 1, sizeof(buffer), file);
		  
		  if(nread==0){
		      printf("ERROR: nothing to read\n");
		      break;
		  }

		  else if(nread<0){
		      printf("ERROR: reading from file\n");
		      exit(-1);
		  }

		  nwrite=write(newfd, buffer, nread);
		  
		  if(nwrite==0){
		      printf("ERROR: nothing to write\n");
		      break;
		  }

		  else if(nwrite<0){
		      printf("ERROR: writing file on socket\n");
		      exit(-1);
		  }
		  sumread+=nread;
		  sumwrite+=nwrite;
		  //printf("%d %d\n",sumread,sumwrite);
		  bzero(buffer, sizeof(buffer));
	      }
	      write(newfd,"\n",1);
	    }
	    fclose(file);
	    bzero(buffer, sizeof(buffer));
// 	    printf("SAIU1\n");
      }	



      else if(!strcmp(buffer,"RQS")){
// 	      printf("AQUI1\n");
	      token = strtok(NULL,s);
	      
	      sid=atoi(token);
	      sprintf(aux,"%d",sid);
	      intlen=strlen(aux);
	      bufflen=strlen(token);

	      if(intlen!=5 || intlen!=bufflen){
		      printf("SID ERRADO\n");
		      write(newfd, "ERR\n", 4);
		      exit(-1);
	      }
	      token=strtok(NULL,s);
	      strcpy(qid,token);
	      
	      if(strlen(token)>24){
		    printf("QID MUITO GRANDE\n");
		    write(newfd, "ERR\n", 4);
		    exit(-1);
	      }
// 	       printf("AQUI2\n");
	      for(i=0;i<5;i++){
		  token=strtok(NULL,s);
		  if(strcmp(token,"A") && strcmp(token,"B") && strcmp(token,"C") && strcmp(token,"D")){
		      printf("RESPOSTAS INVALIDAS\n");
		      write(newfd, "ERR\n", 4);
		      exit(-1);
		  }
		  letters[i]=token[0];
		  
	      }
	      token=strtok(NULL,s);
// 	       printf("AQUI3\n");
	      if(token!=NULL){
		    printf("ERRO\n");
		    write(newfd, "ERR\n", 4);
		    exit(-1);
	      }
// 	      printf("AQUI3\n");
	      
	      sprintf(buffer, "%s%02d%s","T", topicnumber,"quest.txt");
	      file=fopen(buffer,"r+");
	      
	      if(file==NULL){
		printf("Error: opening file\n");
		exit(-1);
	      }
	      sumread=0;
	      find=0;
	      while(!feof(file)){
		  nread=fread(buffer,1, 64,file);
		  sumread+=nread;
		  token=strtok(buffer,s);
		  strcpy(aux,token);
		  if(!strcmp(qid,aux)){
		    fseek(file, sumread-nread, SEEK_SET);
		    fwrite("A",1,1,file);
		    find=1;
		    break;
		  }
		  else
		    printf("ITER %s\n",aux);
		  
	      }
	      if(!find){
		printf("ERROR: already submitted\n");
		exit(-1);
	      }
	      token=strtok(NULL,s);
	      strcpy(aux, token);
	      token=strtok(NULL,s);
	      strcpy(timer, token);
	      token=strtok(NULL,s);
	      strcpy(filename,token);
	      
	      fclose(file);
	      
	      //Verifica se veio dentro da deadline
	      i=verifyDeadline(timer);
	      
	      if(i==-1){
		printf("ERROR: failed deadline\n");
		write(newfd, "-1\n", 3);
		exit(-1);
	      }
	      i=atoi(aux);
// 	      printf("SID %d   DOC %d\n",sid,i);
	      //Verifica se o SID e o request coincidem
	      if(i!=sid){
		  printf("ERROR: permission denied\n");
		  write(newfd, "-2\n", 3);
		  break;
	      }

	     
	      else{
		printf("AQUI %s\n",filename);
		  bzero(aux,sizeof(aux));
		  strncpy(aux,filename,8);
		  strcat(aux,"A.txt");
		  printf("AUX %s\n",aux);
		  file=fopen(aux,"r");
		  if(file==NULL){
		      printf("ERROR: file not found\n");
		      exit(-1);
		  }
		  i=0;	    
		  int score=0;
		  
		  while(i<5) {
		      nread=fread(letter, 1, 1, file);
		      
		      if(nread==0){
			  printf("ERROR: nothing to read\n");
			  break;
		      }

		      else if(nread<0){
			  printf("ERROR: reading from file\n");
			  exit(-1);
		      }
		      if(letters[i]==letter[0])
			score+=20;
		      i+=nread;
		  }
		  
		  sprintf(aux,"%d",sid);
		  strcpy(buffer,aux);
		  strcat(buffer," Score: ");
		  sprintf(aux,"%d",score);
		  strcat(buffer,aux);
		  strcat(buffer,"%");
		  
		  printf("%s\n",buffer);

		  bzero(buffer, sizeof(buffer));
		  bzero(aux,sizeof(aux));
		  
		  strcpy(buffer,"AQS ");
		  strcat(buffer,qid);
		  strcat(buffer," ");
		  sprintf(aux,"%d",score);
		  strcat(buffer,aux);
		  strcat(buffer,"\n");
		  printf("SEND %s\n",buffer);
		
		  nwrite=write(newfd, buffer, 30+strlen(aux));
		  printf("%d\n", nwrite);
		
		  if(nwrite==0){
		      printf("ERROR: nothing to write\n");
		      break;
		  }

		  else if(nwrite<0){
		      printf("ERROR: writing answer on socket\n");
		      exit(-1);
		  }
		  
		  //CONTACT TO ECP (UDP)
		  fd2=socket(AF_INET,SOCK_DGRAM,0); 
		  if(fd2==-1){
		      printf("ERROR connecting to ECP\n");
		      exit(-1);
		  }
		  bzero(buffer, sizeof(buffer));
		  bzero(aux, sizeof(aux));
		  strcpy(buffer,"IQR ");
		  sprintf(aux,"%d",sid);
		  strcat(buffer,aux);
		  strcat(buffer," ");
		  strcat(buffer,qid);
		  strcat(buffer," ");
		  strcat(buffer,topicname);
		  strcat(buffer," ");
		  sprintf(aux,"%d",score);
		  strcat(buffer,aux);
		  strcat(buffer,"\n");
		  
		  hostptr = gethostbyname(ecpname); 
		  
		
		  memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
		  serveraddr.sin_family = AF_INET;
		  serveraddr.sin_addr.s_addr =((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
		  serveraddr.sin_port = htons((u_short)ecpport);

		  addrlen=sizeof(serveraddr);

		  n=sendto(fd2, buffer, sizeof(buffer),0,(struct sockaddr*) &serveraddr, addrlen);

		  if(n<0){
		    printf("ERROR sending to ECP\n");
		    exit(-1);
		  }
		  
		  bzero(buffer, sizeof(buffer));
		  addrlen=sizeof(serveraddr);
		  n=recvfrom(fd2, buffer, sizeof(buffer), 0, (struct sockaddr*)&serveraddr, &addrlen);
		  
		  if(n<0){
		    printf("ERROR receiving from ECP\n");
		    exit(-1);
		  }
		  
		  token=strtok(buffer, s);
		  token=strtok(NULL, s);
		  strtok(token,t);
	      
		  if(strcmp(token, qid)){
		    printf("ERROR: wrong qid on pong\n",token,qid);
		  }
		  

		  close(fd2); 
		  
		  
		
	      }
      }
      else{
	  printf("OUTRA COISA QQ\n");
	  exit(-1);
      }
      
  }
  //parent process
  val=close(newfd);
  while(val==-1 && errno==EINTR);
  if(val==-1)
    exit(-1);
}

  
close(fd);

return 0;
}

int verifyDeadline(char* timer){
    
    time_t tim = time(NULL);
    struct tm tm = *localtime(&tim);
    char month[4];
    int year,day,hour,min,sec,i;
    
    day=atoi(strndup(timer,2));
    strncpy(month,timer+2,3);
    year=atoi(strndup(timer+5,4));
    hour=atoi(strndup(timer+6,2));
    min=atoi(strndup(timer+9,2));
    sec=atoi(strndup(timer+12,2));
    
    if(year<tm.tm_year+1900)
      return -1;
    else if(year>tm.tm_year+1900)
      return 0;
    else{
	for(i=0;i<12;i++){
	  if(!strcmp(months[i],month))
	    break;
	}
	if(i<tm.tm_mon)
	  return -1;
	else if(i>tm.tm_mon)
	  return 0;
	else{
	    if(day<tm.tm_mday)
	      return -1;
	    else if(day>tm.tm_mday)
	      return 0;
	    else{
		if(hour<tm.tm_hour)
		  return -1;
		else if(hour>tm.tm_hour)
		  return 0;
		else{
		  if(min<tm.tm_min)
		    return -1;
		  else if(min>tm.tm_min)
		    return 0;
		  else if(sec<tm.tm_sec)
		    return -1;
		
		}
	
	    
	    }
	}
    }
     printf("AQUI4\n");
    return 0;
}

