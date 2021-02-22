#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

#define BUFFER_SIZE 1024

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	char notify_others[100];
	char aux[4];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
    // lista de clienti
	int clients[10];
	int clients_num = 0;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
FD_ZERO(&read_fds);
FD_ZERO(&tmp_fds);

// Cream socket file descriptor si se asteapta conexiuni
if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
{
    perror("Socket creation failed");
    exit(0); 
    } 

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
    // Completarea informațiilor despre server
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	if (ret < 0) { 
        perror("Bind creation failed"); 
        exit(0); 
    } 

	ret = listen(sockfd, MAX_CLIENTS);
	if (ret < 0) { 
        perror("Listen creation failed"); 
        exit(0); 
    } 

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

    char inputBuffer[BUFFER_SIZE] = {0};
    char outputBuffer[BUFFER_SIZE] = {0};

    while (1) {
		tmp_fds = read_fds;
        // timitem comanda server-ului 
        fgets(outputBuffer, BUFFER_SIZE, stdin);
        // Stergem caracterul \n de la sfarsit
        outputBuffer[strlen(outputBuffer) - 1] = 0;
        // ajută la controlarea mai multor descriptori (de fisiere sau socketi) in acelasi timp
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        
        if (ret < 0) {
          perror("Select creation failed");
          exit(0);
        }
        if (strncmp("unsubscribe", outputBuffer, 12) == 0)
        {
            // Nu mai trimitem mesaje serverului
            break;
        }
        else if (strncmp("subscribe", outputBuffer, 12) == 0)
        {
            //  Primim un mesaj pentru a confirma daca file exista
            recv(sockfd, inputBuffer, BUFFER_SIZE, 0);
            if (strncmp("New client (CLIENT_ID) connected from IP:PORT.", inputBuffer, 47) == 0)
            {
                fprintf(stdin, "[WARN] Server refused to send this file. Maybe file does not exist.\n");
                continue;
            }

            for (i = 0; i <= fdmax; i++)
            {
                if (FD_ISSET(i, &tmp_fds))
                {
                    if (i == sockfd)
                    {
                        // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                        // pe care serverul o accepta
                        clilen = sizeof(cli_addr);
                        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                        if (ret < 0)
                        {
                            perror("Accept creation failed");
                            exit(0);
                        }

                        // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                        FD_SET(newsockfd, &read_fds);
                        if (newsockfd > fdmax)
                        {
                            fdmax = newsockfd;
                        }

                        printf("New client %s connected from  %d:%d\n",
                               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

                        // adauga-l in lista de clients
                        clients[clients_num++] = newsockfd;
                        sprintf(notify_others, "[UPDATE] New client: %d\n", newsockfd);

                        char others[100] = "Other clients are: ";

                        for (int j = 0; j < clients_num - 1; j++)
                        {
                            sprintf(aux, "%d", clients[j]);
                            strcat(others, aux);
                            strcat(others, " ");
                            send(clients[j], notify_others, strlen(notify_others), 0);
                        }
                        if (clients_num > 1)
                        {
                            strcat(others, "\n");
                            send(newsockfd, others, strlen(others), 0);
                        }
                    }
                    else
                    {
                        // s-au primit date pe unul din socketii de client,
                        // asa ca serverul trebuie sa le receptioneze
                        memset(inputBuffer, 0, BUFLEN);
                        n = recv(i, inputBuffer, sizeof(buffer), 0);
                        if (n < 0)
                        {
                            perror("Recv creation failed");
                            exit(0);
                        }

                        if (n == 0)
                        {
                            // conexiunea s-a inchis
                            close(i);

                            sprintf(notify_others, "[UPDATE] Client %d closed connection\n", i);

                            for (int j = 0; j < clients_num; j++)
                            {
                                if (j != i)
                                {
                                    send(clients[j], notify_others, strlen(notify_others), 0);
                                }
                            }

                            // se scoate din multimea de citire socketul inchis
                            FD_CLR(i, &read_fds);
                        }
                        else
                        {
                            int dest = atoi(inputBuffer);
                            send(dest, inputBuffer + 2, strlen(inputBuffer) - 2, 0);
                            printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, inputBuffer);
                        }
                    }
                }
            }
	}

	close(sockfd);

	return 0;
}
