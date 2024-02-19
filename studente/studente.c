/*
* Traccia - Università
*      Scrivere un'applicazione client/server parallelo per gestire gli esami universitari
*
* Segreteria:
*      x Inserisce gli esami sul server dell'università (salvare in un file o conservare in memoria il dato)
*      x Inoltra la richiesta di prenotazione degli studenti al server universitario
*      x Fornisce allo studente le date delgi esami diposnibili per l'esame scelto dallo studente
* Studente:
*      x Chiede alla segreteria se ci siano esami disponibili per un corso
*      x Invia una richiesta di prenotazione di un esame alla segreteria
* Server universitario:
*      x Riceve l'aggiunta di nuovi esami
*      x Riceve la prenotazione di un esame
*      x Il server universitario ad ogni richiesta di prenotazione invia alla segreteria il numero di prenotazione progressivo
*          assegnato allo studente e la segreteria a sua volta lo inoltra allo studente
*/


#include <stdio.h>
#include <stdlib.h>
#include "wrapper.h"

struct Exam {
    char name[40];
    char date[25];
    short cfu;
};

struct Request {
    short requestType;
    struct Exam exam;
};

struct Request getRequestType(struct Request *availableExamRequest);

void printRecivedExams(struct Exam *, int);

struct Request getExamName(struct Request *availableExamRequest);

void sendRequestedExamToSegreteria(int socketFD, struct Request *availableExamRequest);

int reciveExamsCount(int socketFD, int *examsSelectedByNameCount);

void reciveExamsList(int socketFD, int examsSelectedByNameCount, struct Exam *recivedExam);

int connectToSegreteria(int socketFD, struct sockaddr_in *serverSegreteriaAddress);

int getBookingId(int socketFD, int *bookingId);

void sendExamToBook(int socketFD, struct Exam *examToBook);

void sendMatricola(int socketFD, const char *matricola);

int main() {
    int socketFD;
    struct sockaddr_in serverSegreteriaAddress;

    //chiede all'utente il tipo di richiesta da effettuare
    struct Request availableExamRequest = {};
    availableExamRequest = getRequestType(&availableExamRequest);

    if (availableExamRequest.requestType == 1) {
        availableExamRequest = getExamName(&availableExamRequest);

        if (availableExamRequest.exam.name[0] != '\000') {

            //crea il socket per la comunicazione con la segreteria
            socketFD = connectToSegreteria(socketFD, &serverSegreteriaAddress);

            //inoltra alla segreteria l'esame richiesto
            sendRequestedExamToSegreteria(socketFD, &availableExamRequest);

            //riceve dalla segreteria il numero di esami disponibili
            int examsSelectedByNameCount = 0;
            struct Exam *recivedExam;
            examsSelectedByNameCount = reciveExamsCount(socketFD, &examsSelectedByNameCount);
            if (examsSelectedByNameCount > 0) {
                //riceve dalla segreteria la lista di esami disponibili
                recivedExam = (struct Exam *) malloc(sizeof(struct Exam) * examsSelectedByNameCount);
                reciveExamsList(socketFD, examsSelectedByNameCount, recivedExam);

                printRecivedExams(recivedExam, examsSelectedByNameCount);

                //
                int examToBookCode = 0;
                char matricola[11] = "0123456789";
                printf("\nInserisci il codice dell'esame al quale vuoi prenotarti (digita '0' per uscire): ");
                scanf("%d", &examToBookCode);

                printf("\nInserisci la matricola: ");
                scanf("%s", matricola);

                struct Exam examToBook;
                examToBook = recivedExam[examToBookCode - 1];
                if (examToBookCode != 0) {
                    //iniva alla segreteria l'esame al quale lo studente vuole prenotarsi
                    sendExamToBook(socketFD, &examToBook);
                    sendMatricola(socketFD, matricola);

                    int bookingId;
                    bookingId = getBookingId(socketFD, &bookingId);

                    printf("Prenotato con successo, il tuo numero di prenotazione e': %d\n", bookingId);

                }
                free(recivedExam);
                //
            } else {
                printf("Non ci sono esami disponibili per questo corso");
                return 0;
            }
        }

    }

    close(socketFD);
}

void sendMatricola(int socketFD, const char *matricola) {
    //iniva alla segreteria la matricola dello studente

    int matsize = sizeof(*matricola);

    if (write(socketFD, matricola, sizeof(char) * 11) != sizeof(char) * 11) {
        perror("Write error 2");
        exit(1);
    }
}

void sendExamToBook(int socketFD, struct Exam *examToBook) {
    if (write(socketFD, examToBook, sizeof(struct Exam)) != sizeof(struct Exam)) {
        perror("Write error 1");
        exit(1);
    }
}

int getBookingId(int socketFD, int *bookingId) {//legge il numero di prenotazione progressivo assegnatogli
    if (read(socketFD, bookingId, sizeof(int)) != sizeof(int)) {
        perror("read error 1");
        exit(-1);
    }
    return (*bookingId);
}

int connectToSegreteria(int socketFD, struct sockaddr_in *serverSegreteriaAddress) {
    socketFD = Socket(AF_INET, SOCK_STREAM, 0);

    //inizializza la porta su cui è in ascolto la segreteria e la converte in network order
    (*serverSegreteriaAddress).sin_family = AF_INET;
    (*serverSegreteriaAddress).sin_port = htons(2000);

    //converte l'indirizzo della segreteria in un indirizzo di rete scritto in network order e lo salva in sin_addr
    if (inet_pton(AF_INET, "127.0.0.1", &(*serverSegreteriaAddress).sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", "127.0.0.1");
        exit(1);
    }

    //connette il socketFD all'indirizzo serverSegreteriaAddress
    Connect(socketFD, (struct sockaddr *) serverSegreteriaAddress, sizeof(*serverSegreteriaAddress));
    return socketFD;
}

void reciveExamsList(int socketFD, int examsSelectedByNameCount, struct Exam *recivedExam) {
    if (read(socketFD, recivedExam, sizeof(struct Exam) * examsSelectedByNameCount) !=
        sizeof(struct Exam) * examsSelectedByNameCount) {
        perror("read error 2");
        exit(-1);
    }
}

int reciveExamsCount(int socketFD, int *examsSelectedByNameCount) {
    if (read(socketFD, examsSelectedByNameCount, sizeof(*examsSelectedByNameCount)) !=
        sizeof(*examsSelectedByNameCount)) {
        perror("read error 3");
        exit(-1);
    }
    return (*examsSelectedByNameCount);
}

void sendRequestedExamToSegreteria(int socketFD, struct Request *availableExamRequest) {
    if (write(socketFD, availableExamRequest, sizeof(*availableExamRequest)) != sizeof(*availableExamRequest)) {
        perror("Write error 3");
        exit(1);
    }
}

struct Request getExamName(struct Request *availableExamRequest) {
    printf("Nome dell'esame da ricercare: ");
    fgets((*availableExamRequest).exam.name, 40, stdin);
    (*availableExamRequest).exam.name[strlen((*availableExamRequest).exam.name) - 1] = '\000';
    fflush(stdin);
    return (*availableExamRequest);
}

void printRecivedExams(struct Exam *recivedExam, int n) {
    printf("------------------------------------------------------------------------\n");
    printf("Code\t\tData\t\t\tCFU\t\tNome\t\t\n");
    printf("------------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        printf("%d\t\t%s\t\t%d\t\t%s\n", i + 1, recivedExam[i].date, recivedExam[i].cfu, recivedExam[i].name);
        printf("------------------------------------------------------------------------\n");
    }
}

struct Request getRequestType(struct Request *availableExamRequest) {
    printf("Sei connesso alla segreteria!\n");
    printf("1 - Lista esami disponibili\n2 - Prenota Esame\n");
    printf("Scelta:\t");
    scanf("%hd", &(*availableExamRequest).requestType);
    getchar();
    return (*availableExamRequest);
}
