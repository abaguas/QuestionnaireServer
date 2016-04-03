#ifndef ECP_H_
#define ECP_H_

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
#include <readline/readline.h>
#include <readline/history.h>
#include <time.h>


#define PORT 					58028
#define MAX_PORT_NUMBER			5
#define MAX_IP_ADRESS			16
#define MAX_TOPIC_NAME			25
#define MAX_QUESTIONNAIRE_ID	24
#define MAX_SCORE				3
#define MAX_STUDENT_ID			5
#define MAX_REQUEST 			3

typedef struct User {
	int		id;
	int		total_score;
	int		number_questionnaires;
}*User;


typedef struct Questionnaire {
	char*		id;
	char*		topic_name;
	int			total_score;
	int			number_questionnaires;
}*Questionnaire;


extern Questionnaire *questionnaires_info;
extern User *users_info;
extern int questionnaires_info_size;
extern int users_info_size;
extern int lines_stats;
extern int sem_id;
extern char buffer[MAX_REQUEST+MAX_STUDENT_ID+MAX_QUESTIONNAIRE_ID+MAX_TOPIC_NAME+MAX_SCORE+6+5];
extern int send_EOF;
extern char buffer_aux[MAX_REQUEST+MAX_STUDENT_ID+MAX_QUESTIONNAIRE_ID+MAX_TOPIC_NAME+MAX_SCORE+6+5];


void send_topics(int fd, int addrlen, struct sockaddr_in clientaddr, int nr_topics);
void send_TES_info(int fd, int nr_topics, struct sockaddr_in clientaddr);
int valid_file(int fd, int addrlen, struct sockaddr_in clientaddr);
void receive_score(int fd, int addrlen, struct sockaddr_in clientaddr);
void request_handler(int addrlen, struct sockaddr_in serveraddr, struct sockaddr_in clientaddr, int fd);
int valid_fields_TES(char **saver);
int verify_number_words(int n);
int verify_len_number(char* string);

Questionnaire questionnaire_highest_score();

Questionnaire questionnaire_most_answered();

User user_highest_score();

User user_most_active();

#endif
