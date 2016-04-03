#include "ECP.h"

//VERIFICAR SE OS FICHEIROS NAO ESTAO CERTOS (formato, bullet proof, robusto)
//LIMPAR SEMPRE OS BUFFERS
//porquê que depois de fazer close o fd ainda tem valor?
//é preciso verificar erro no malloc?
//fazer testes à flag -p
//quando são feitos muitos pedidos esta cena crasha
//correr o valgrind
//se o user id for 00003 deve dar merda no length do numero
//ver linha 428
//o que é um IP válido?
//qualquer erro no ficheiro topics leva um EOF?

Questionnaire *questionnaires_info;
User *users_info;
int questionnaires_info_size;
int users_info_size;
int sem_id;
char buffer[MAX_REQUEST+MAX_STUDENT_ID+MAX_QUESTIONNAIRE_ID+MAX_TOPIC_NAME+MAX_SCORE+6+5]; //4 espaços, + 1 \n, +1 \0, mais 5 extra
char buffer_aux[MAX_REQUEST+MAX_STUDENT_ID+MAX_QUESTIONNAIRE_ID+MAX_TOPIC_NAME+MAX_SCORE+6+5];
int lines_stats;
int send_EOF;
int len;

int main(int argc, char *argv[]){
	int i;
	int fd, ecp_port;
	struct sockaddr_in serveraddr, clientaddr;
	int addrlen;
	int questionnaires_info_size = 0;
	int users_info_size = 0;
	lines_stats = 0;
	ecp_port = PORT;
	//inicializar o ficheiro stats
	FILE *file = fopen("stats.txt", "w");
	
	if (file==NULL){
		perror("Error creating stats.txt\n");
	}
	fclose(file);
	
	questionnaires_info = (Questionnaire*)malloc(sizeof(Questionnaire));
	users_info = (User*)malloc(sizeof(User));
	
	if (argc == 3){
		if (!strcmp(argv[1], "-p")) {
			if ((ecp_port>9999 && ecp_port<100000) && (verify_len_number(argv[2]))){
				ecp_port = atoi(argv[2]);
			}
			else {
				printf("invalid port number\n");
				exit(-1);
			}
		}
		else{
			printf("invalid flag\n");
			exit(-1);
		}
	}
	else if (argc!=1){
		printf("invalid number of arguments\n");
		exit(-1);
	}

	if ((fd=socket(AF_INET,SOCK_DGRAM,0)) == -1){
		perror("error in socket system call\n");
		exit(-1);
	}
	
	memset((void*)&serveraddr,(int)'\0',sizeof(serveraddr));
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons ((u_short)ecp_port);
		
	if (bind(fd,(struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1){
		perror("error in bind system call\n");
		exit(-1);
	};
	
	while(1){
		addrlen=sizeof(clientaddr);
		bzero(buffer_aux, sizeof(buffer_aux));
		if ((len = recvfrom(fd, buffer_aux, sizeof(buffer_aux), 0, (struct sockaddr*)&clientaddr, (socklen_t*) &addrlen)) == -1){
			perror("error in recvfrom system call\n");
			exit(-1);
		}
		buffer_aux[MAX_REQUEST+MAX_STUDENT_ID+MAX_QUESTIONNAIRE_ID+MAX_TOPIC_NAME+MAX_SCORE+6+4]='\0';
		strcpy(buffer, buffer_aux);
		if ((strlen(buffer)) > MAX_REQUEST+MAX_STUDENT_ID+MAX_QUESTIONNAIRE_ID+MAX_TOPIC_NAME+MAX_SCORE+5){ //4 espacos e 1 \n
			sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		}
		request_handler(addrlen, serveraddr, clientaddr, fd);
		
	}
	for (i=0; i<questionnaires_info_size; i++){
		free(questionnaires_info[i]->id);
		free(questionnaires_info[i]->topic_name);
		free(questionnaires_info[i]);
	}
	free(questionnaires_info);
	for (i=0; i<users_info_size; i++){
		free(users_info[i]);
	}
	
	free(users_info);
	
	return 0;
}



void request_handler(int addrlen, struct sockaddr_in serveraddr, struct sockaddr_in clientaddr, int fd){
	char s[2]=" ";
	int send_EOF=0;
	char *token;
	int nr_topics=0;
		
	if (!(buffer[strlen(buffer)-1]=='\n')){
		sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		return;
	}
	
	buffer[strlen(buffer)-1] = '\0';
	buffer_aux[strlen(buffer_aux)-1] = '\0';
	
	if (buffer[0]=='\0'){
		sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		return;
	}
	token = strtok(buffer, s);
	
	if ((!strcmp(token, "TQR")) && (nr_topics = valid_file(fd, addrlen, clientaddr)) && (verify_number_words(1))){
		send_topics(fd, addrlen, clientaddr, nr_topics);
	}
	else if ((!strcmp(token, "IQR")) && verify_number_words(5)){
		receive_score(fd, addrlen, clientaddr);
	}
	else if ((!strcmp(token, "TER"))&& (nr_topics = valid_file(fd, addrlen, clientaddr)) && (verify_number_words(2))){
		send_TES_info(fd, nr_topics, clientaddr);
	}
	else if (send_EOF){
		send_EOF = 0;
		sendto(fd, "EOF\n", strlen("EOF\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
	}
	else {
		sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
	}
}

int verify_number_words(int n){
	int i;
	int counter=0;
	
	for (i=0; i<(strlen(buffer_aux)); i++){
		if (buffer_aux[i]=='\n'){
			return 0;
		}
		else if (buffer_aux[i]==' '){
			counter ++;
		}
	}
	if (counter!=(n-1)){
		return 0;
	}
	return 1;
}

void receive_score(int fd, int addrlen, struct sockaddr_in clientaddr){
	int i=0;
	char *saver[4];
	char s[2]=" ";
	char *token;
	char message[MAX_REQUEST + MAX_QUESTIONNAIRE_ID+3]; //espaço, \n e \0
	int counter=0;
	Questionnaire q_highest_score, q_most_answered;
	User u_highest_score, u_most_active;
	int flag = 0, first=0;
	FILE *file = fopen("stats.txt", "r+");
	
	if (file==NULL){
		perror("Error opening the file stats.txt\n");
	}
	
	bzero(message, sizeof(message));
	
	//não é lido o AWI
	token = strtok(NULL, s);
	
	/* walk through tokens tokens */
	for (i=0; i<4; i++){
		saver[i] = token;
		token = strtok(NULL, s);
	}
	
	//verifica se o pedido foi bem formulado
	if (!valid_fields_TES(saver)){
		sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		return;
	}
	
	//posicionar cursor no final das linhas correspondentes às submissoes
	//para escrever por cima das estatisticas
	fseek(file, 0, SEEK_SET);
	while (counter<lines_stats){
		if (fgetc(file)=='\n'){
			counter++;
		}
	}
	
	for (i=0; i<3; i++){
		fwrite(saver[i], sizeof(char), strlen(saver[i]), file);
		fwrite(" ", sizeof(char), 1, file);
	}
	fwrite(saver[3], sizeof(char), strlen(saver[3]), file);
	
	fwrite("\n", sizeof(char), 1, file);
	lines_stats++;
		
	//armazenar dados questionarios
	if (questionnaires_info_size==0){
		questionnaires_info_size++;
		first=1;
	}
	else {
		//procurar nome nos questionarios ja guardados
		//flag guarda a posicao onde está guardado o questionario
		for (i=0; i<questionnaires_info_size; i++){
			if (!strcmp(questionnaires_info[i]->topic_name, saver[2])){
				flag = i+1;
				break;
			}
		}
		if (flag){
			questionnaires_info[flag-1] -> total_score += atoi(saver[3]);
			questionnaires_info[flag-1] -> number_questionnaires ++;
		}
	}	
	if (!flag){
		//incremento contador
		if (!first){
			questionnaires_info_size++;
		}
		//reservo memoria
		questionnaires_info = (Questionnaire*)realloc(questionnaires_info, questionnaires_info_size*sizeof(Questionnaire));
		questionnaires_info[questionnaires_info_size-1] = (Questionnaire)malloc(sizeof(struct Questionnaire));
		questionnaires_info[questionnaires_info_size-1]->topic_name = (char*)malloc(strlen(saver[2])*sizeof(char));
		//preencho memoria
		strcpy(questionnaires_info[questionnaires_info_size-1] -> topic_name, saver[2]);
		questionnaires_info[questionnaires_info_size-1] -> total_score = atoi(saver[3]);
		questionnaires_info[questionnaires_info_size-1] -> number_questionnaires = 1;
	}
	
	flag = 0;
	first = 0;

	//armazenar dados users
	if (users_info_size==0){
		users_info_size++;
		first = 1;
	}
	else{
		//1-verificar se user ja existe
		for (i=0; i<users_info_size; i++){
			if (users_info[i]-> id == atoi(saver[0])){
				flag = i+1;
				break;
			}
		}	
		if (flag){
			users_info[flag-1] -> total_score += atoi(saver[3]);
			users_info[flag-1] -> number_questionnaires ++;
		}
	}	
	if (!flag) {
		//incremento contador
		if (!first){
			users_info_size++;
		}
		//reservo memoria
		users_info = (User*)realloc(users_info, users_info_size*sizeof(User));
		users_info[users_info_size-1] = (User)malloc(sizeof(struct User));
		//preencho memoria
		users_info[users_info_size-1] -> id = atoi(saver[0]);
		users_info[users_info_size-1] -> total_score = atoi(saver[3]);
		users_info[users_info_size-1] -> number_questionnaires = 1;
	}
	
	
	//calcula e escreve as estatisticas
	q_highest_score = questionnaire_highest_score();
	fwrite("Questionario com melhor pontuacao: ", sizeof(char), strlen("Questionario com melhor pontuacao: "), file);
	fwrite(q_highest_score->topic_name, sizeof(char), strlen(q_highest_score->topic_name), file);
	fwrite("\n", sizeof(char), 1, file);
	
	
	q_most_answered = questionnaire_most_answered();
	fwrite("Questionario mais respondido: ", sizeof(char), strlen("Questionario mais respondido: "), file);
	fwrite(q_most_answered->topic_name, sizeof(char), strlen(q_most_answered->topic_name), file);
	fwrite("\n", sizeof(char), 1, file);
	
	
	u_highest_score = user_highest_score();
	fwrite("User com melhor pontuacao: ", sizeof(char), strlen("User com melhor pontuacao: "), file);
	fprintf(file, "%d", u_highest_score->id);
	fwrite("\n", sizeof(char), 1, file);
	
	
	u_most_active = user_most_active();
	fwrite("User com mais questionarios respondidos: ", sizeof(char), strlen("User com mais questionarios respondidos: "), file);
	fprintf(file, "%d", u_most_active->id);
	fwrite("\n", sizeof(char), 1, file);
	
	
	fclose(file);
	
	strcat(message, "AWI ");
	strcat(message, saver[1]);
	strcat(message, "\n");
	
	sendto(fd, message, strlen(message), 0,(struct sockaddr*)&clientaddr, addrlen);
}

int valid_file(int fd, int addrlen, struct sockaddr_in clientaddr){
	FILE *file = fopen("topics.txt", "r");
	int n_input=0, counter=0;
	char character;
	char aux[MAX_TOPIC_NAME+1]; //+ '\0'
	
	if (file==NULL){
		send_EOF = 1;
		return 0;
	}
	
	n_input = fscanf(file, "%s", aux);
	while(n_input != EOF){
		counter++;
		if (n_input==0 || n_input > MAX_TOPIC_NAME){
			send_EOF = 1;
			return 0;
		}
		character = (char)fgetc(file);
		if (character != ' '){
			send_EOF = 1;
			return 0;
		}
		n_input = fscanf(file, "%s", aux);
		if (n_input==0 || n_input > MAX_IP_ADRESS){
			send_EOF = 1;
			return 0;
		}
		character = (char)fgetc(file);
		if (character != ' '){
			send_EOF = 1;
			return 0;
		}
		
		n_input = fscanf(file, "%s", aux);
		if ((n_input==0 || atoi(aux) > 99999 || atoi(aux) <10000) && (!verify_len_number(aux))){
			send_EOF = 1;
			return 0;
		}
		character = (char)fgetc(file);
		if (character != '\n'){
			send_EOF = 1;
			return 0;
		}
		bzero(aux, sizeof(aux));
		n_input = fscanf(file, "%s", aux);
	}
	return counter;
}

void send_TES_info(int fd, int nr_topics, struct sockaddr_in clientaddr){
	int i=0, topic_number;
	FILE *file = fopen("topics.txt", "r");
	char address[MAX_REQUEST+MAX_IP_ADRESS+MAX_PORT_NUMBER+4];  //+2 espaços, +1\n, +1\0
	int addrlen;
	char s[2] = " ";
	char* token;
	char aux[MAX_TOPIC_NAME+1];
	
	addrlen = sizeof(clientaddr);
	bzero(address, sizeof(address));
	bzero(aux, sizeof(aux));
	
	if (file==NULL){
		sendto(fd, "EOF\n", strlen("EOF\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		return;
	}
	
	token = strtok(NULL, s);
	topic_number = atoi(token);
	if (token[0]=='0'){
		if (topic_number < 1){
			sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
			return;
		}
	}
	else{
		if (!verify_len_number(token)){
			sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
			return;
		}
	}
	
		
	//verifica se o pedido está bem formulado, da forma Txx e se o numero do topico existe no ficheiro
	if ((topic_number > nr_topics) || ((strlen(token)!=2) && (strlen(token)!=1))){
		sendto(fd, "ERR\n", strlen("ERR\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		return;
	}
	
	strcat(address, "AWTES ");
	
	for(i=0; i<topic_number-1; i++){
		fscanf(file, "%s", aux);
		fscanf(file, "%s", aux);
		fscanf(file, "%s", aux);
	}
	fscanf(file, "%s", aux);//descartar topic name
	
	fscanf(file, "%s", aux); //ip address
	
	strcat(address, aux);
	
	strcat(address, " ");
	
	fscanf(file, "%s", aux); //port number
	strcat(address, aux);
	strcat(address, "\n");
	
	sendto(fd, address, strlen(address), 0, (struct sockaddr*)&clientaddr, addrlen);
	
	fclose(file);
}

void send_topics(int fd, int addrlen, struct sockaddr_in clientaddr, int nr_topics){
	char topic_names[99*MAX_TOPIC_NAME+MAX_REQUEST+99+2]; //99*' ' + '\n' + '\0'
	char aux[MAX_TOPIC_NAME+1];
	
	bzero(topic_names, sizeof(topic_names));
	bzero(aux, sizeof(aux));
	FILE *file = fopen("topics.txt", "r");
	if (file==NULL){
		sendto(fd, "EOF\n", strlen("EOF\n"), 0,(struct sockaddr*)&clientaddr, addrlen);
		return;
	}
	else {
		
		strcat(topic_names, "AWT ");
		sprintf(aux, "%d", nr_topics);
		strcat(topic_names, aux);
		
		while(fscanf(file, "%s", aux)!= EOF){
			strcat(topic_names, " ");
			strcat(topic_names, aux);
			fscanf(file, "%s", aux); //descarto IP
			fscanf(file, "%s", aux); //descarto port
		}
		strcat(topic_names, "\n");
		
		sendto(fd, topic_names, strlen(topic_names) ,0,(struct sockaddr*)&clientaddr, addrlen);
		fclose(file);
	}
}

Questionnaire questionnaire_highest_score(){
	Questionnaire q;
	int i, highest;
	int score = 0;
	for (i=0; i<questionnaires_info_size; i++){
		q = questionnaires_info[i];
		if ((q->total_score / q->number_questionnaires) > score){
			highest = i;
		}
	}
	return questionnaires_info[highest];
}

Questionnaire questionnaire_most_answered(){
	Questionnaire q;
	int i, maximum = 0;
	for (i=0; i<questionnaires_info_size; i++){
		q = questionnaires_info[i];
		if ((q->number_questionnaires) > maximum){
			maximum = i;
		}
	}
	return questionnaires_info[maximum];	
}


User user_highest_score(){
	User u;
	int i, highest;
	int score = 0;
	for (i=0; i<users_info_size; i++){
		u = users_info[i];
		if ((u->total_score / u->number_questionnaires) > score){
			highest = i;
		}
	}
	return users_info[highest];
}

User user_most_active(){
	User u;
	int i, maximum=0;
	for (i=0; i<users_info_size; i++){
		u = users_info[i];
		if (u->number_questionnaires > maximum){
			maximum = i;
		}
	}
	return users_info[maximum];
}

int valid_fields_TES(char** saver){
	if (!verify_len_number(saver[0])){
		return 0;
	}
	if (!verify_len_number(saver[3])){
		return 0;
	}
	if (atoi(saver[0]) < 10000 || atoi(saver[0]) > 99999){
		return 0;
	}//user_id  numero 5 caracteres
	if (strlen(saver[1]) > MAX_QUESTIONNAIRE_ID){
		return 0;
	}//  q_id string até 24 caracteres
	if (strlen(saver[2])>MAX_TOPIC_NAME){
		return 0;
	}//  topic name  string ate 25 caracteres
	if (atoi(saver[3]) < 0 || atoi(saver[3]) > 100){
		return 0;
	}//  score  inteiro entre 0 e 100
	return 1;
}

int verify_len_number(char* string){
	char a[5];
	sprintf(a, "%d", atoi(string));
	if (strlen(string) == strlen(a)){
		return 1;
	}
	return 0;
}
