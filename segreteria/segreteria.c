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

struct Exam *getAndSendExams(int connFD, int socketFD);

void addExam();

void sendAddedExamToServer(struct Exam);

void retriveAvailableExams(struct Exam, int);

void sendRequestedExamToServer(int, struct Request);

int reciveSelectedExamsCount(int);

void reciveSelectedExamsList(struct Exam **, int, int);

int startListeningForStudents(int listenFD, struct sockaddr_in *segreteriaAddress);

int connectToUniversita(int socketFD, struct sockaddr_in *serverUniAddress);

void sendSelectedExamsCountToStudente(int connFD, int *examsSelectedByNameCount);

void sendSelectedExamsToStudente(int connFD, int examsSelectedByNameCount, const struct Exam *recivedExam);

void getAndSendExamToBook(int connFD, int socketFD, struct Exam *examToBook);

void getAndSendMatricola(int connFD, int socketFD);

void getAndSendBookingId(int connFD, int socketFD);

int main() {
    int adding = 1;

    int listenFD, connFD;
    struct sockaddr_in segreteriaAddress;
    struct Request recivedRequest;
    pid_t pid1, pid2;

    if ((pid1 = fork()) < 0) {
        perror("fork error");
        exit(-1);
    }

    if (pid1 == 0) {
        //crea il socket listenFD per l'ascolto
        listenFD = startListeningForStudents(listenFD, &segreteriaAddress);

        while (1) {

            //accetta la connessione in ingresso
            connFD = Accept(listenFD, (struct sockaddr *) NULL, NULL);

            if ((pid2 = fork()) < 0) {
                perror("fork error");
                exit(-1);
            }

            if (pid2 == 0) {
                //riceve la richiesta da parte dello studente
                if (read(connFD, &recivedRequest, sizeof(struct Request)) != sizeof(struct Request)) {
                    perror("read error 1");
                    exit(-1);
                }

                //se la richiesta è di tipo 1 allora lo studente ha richiesto la lista degli esami disponibili per un corso
                if (recivedRequest.requestType == 1) {
                    retriveAvailableExams(recivedRequest.exam, connFD);
                }

                close(connFD);
                exit(0);
            } else {
                close(connFD);
            }
        }
    } else {
        while (adding) {
            addExam();
            printf("premi '0' per uscire oppure '1' per aggiungere un altro esame: ");
            scanf("%d", &adding);
            getchar();
        }
    }
    return 0;
}

int startListeningForStudents(int listenFD, struct sockaddr_in *segreteriaAddress) {
    listenFD = Socket(AF_INET, SOCK_STREAM, 0);

    (*segreteriaAddress).sin_family = AF_INET;
    (*segreteriaAddress).sin_addr.s_addr = htonl(INADDR_ANY);
    (*segreteriaAddress).sin_port = htons(2000);

    int enable = 1;
    setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    //assegna l'indirizzo serverAddress al socket listenFD
    Bind(listenFD, (struct sockaddr *) segreteriaAddress, sizeof(*segreteriaAddress));

    //mette il socket in modalità di ascolto in attesa di nuove connessioni, con un massimo di 1025 client in attesa di essere accettati
    Listen(listenFD, 1024);
    return listenFD;
}


void retriveAvailableExams(struct Exam exam, int connFD) {
    int socketFD;
    struct sockaddr_in serverUniAddress;
    struct Request retriveAvailableExamRequest;

    retriveAvailableExamRequest.requestType = 2;
    retriveAvailableExamRequest.exam = exam;

    //crea il socket per la comunicazione con il server università
    socketFD = connectToUniversita(socketFD, &serverUniAddress);

    //inoltra al server l'esame che l'utente ha richiesto
    sendRequestedExamToServer(socketFD, retriveAvailableExamRequest);
    struct Exam *recivedExam = getAndSendExams(connFD, socketFD);

    struct Exam examToBook;
    getAndSendExamToBook(connFD, socketFD, &examToBook);

    getAndSendMatricola(connFD, socketFD);

    getAndSendBookingId(connFD, socketFD);

    //chiude la connsessione con il server
    close(socketFD);
    free(recivedExam);
}

void getAndSendBookingId(int connFD, int socketFD) {
    int bookingId;
    //legge il numero di prenotazione progressivo assegnato allo studente
    if (read(socketFD, &bookingId, sizeof(int)) != sizeof(int)) {
        perror("read error 4");
        exit(-1);
    }

    //invia allo studente il numero di prenotazione progressivo
    if (write(connFD, &bookingId, sizeof(int)) != sizeof(int)) {
        perror("write error 3");
        exit(-1);
    }
}

void getAndSendMatricola(int connFD, int socketFD) {
    char matricola[11] = "0123456789";
    //legge la matricola dello studente0
    if (read(connFD, matricola, sizeof(matricola)) != sizeof(matricola)) {
        perror("read error 3");
        exit(-1);
    }

    //invia al server la matricola dello studente
    if (write(socketFD, matricola, sizeof(matricola)) != sizeof(matricola)) {
        perror("write error 2");
        exit(-1);
    }
}

void getAndSendExamToBook(int connFD, int socketFD, struct Exam *examToBook) {//legge l'esame da prenotare
    if (read(connFD, examToBook, sizeof(struct Exam)) != sizeof(struct Exam)) {
        perror("read error 2");
        exit(-1);
    }

    //invia al server l'esame da prenotare
    if (write(socketFD, examToBook, sizeof(struct Exam)) != sizeof(struct Exam)) {
        perror("write error 1");
        exit(-1);
    }
}

struct Exam *getAndSendExams(int connFD, int socketFD) {//riceve dal server il nuermo degli esami selezionati
    int examsSelectedByNameCount = 0;
    struct Exam *recivedExam;
    examsSelectedByNameCount = reciveSelectedExamsCount(socketFD);

    if (examsSelectedByNameCount > 0) {
        //riceve dal server la lista di esami richiesti dallo studente
        reciveSelectedExamsList(&recivedExam, examsSelectedByNameCount, socketFD);

        //invia allo studente il numero di esami disponibili
        sendSelectedExamsCountToStudente(connFD, &examsSelectedByNameCount);

        //invia allo studente la lista di esami disponibili
        sendSelectedExamsToStudente(connFD, examsSelectedByNameCount, recivedExam);
    } else {
        //invia allo studente il numero di esami disponibili
        sendSelectedExamsCountToStudente(connFD, &examsSelectedByNameCount);
        exit(-1);
    }

    return recivedExam;
}

void sendSelectedExamsToStudente(int connFD, int examsSelectedByNameCount, const struct Exam *recivedExam) {
    if (write(connFD, recivedExam, sizeof(struct Exam) * examsSelectedByNameCount) !=
        sizeof(struct Exam) * examsSelectedByNameCount) {
        perror("Write error 4");
        exit(1);
    }
}

void sendSelectedExamsCountToStudente(int connFD, int *examsSelectedByNameCount) {
    if (write(connFD, examsSelectedByNameCount, sizeof(int)) != sizeof(int)) {
        perror("Write error 5");
        exit(1);
    }
}

//riceve dal server università la lista di esami selezionati
void reciveSelectedExamsList(struct Exam **recivedExam, int examsSelectedByNameCount, int socketFD) {
    *recivedExam = (struct Exam *) malloc(sizeof(struct Exam) * examsSelectedByNameCount);
    if (read(socketFD, *recivedExam, sizeof(struct Exam) * examsSelectedByNameCount) !=
        sizeof(struct Exam) * examsSelectedByNameCount) {
        perror("read error 5");
        exit(-1);
    }

}

//riceve dal server università il numero di esami selezionati
int reciveSelectedExamsCount(int socketFD) {
    int examsSelectedByNameCount;
    if (read(socketFD, &examsSelectedByNameCount, sizeof(int)) != sizeof(int)) {
        perror("read error 6");
        exit(-1);
    }
    return examsSelectedByNameCount;
}

//inoltra al server l'esame che lo studente ha richiesto
void sendRequestedExamToServer(int socketFD, struct Request retriveAvailableExamRequest) {
    if (write(socketFD, &retriveAvailableExamRequest, sizeof(retriveAvailableExamRequest)) !=
        sizeof(retriveAvailableExamRequest)) {
        perror("Write error 6");
        exit(1);
    }
}

void addExam() {

    struct Exam exam;

    printf("Nome corso: ");
    fgets(exam.name, 40, stdin);
    exam.name[strlen(exam.name) - 1] = '\000';
    fflush(stdin);

    printf("Data esame: ");
    fgets(exam.date, 25, stdin);
    exam.date[strlen(exam.date) - 1] = '\000';
    fflush(stdin);

    printf("CFU: ");
    scanf("%hd", &exam.cfu);
    fflush(stdin);

    sendAddedExamToServer(exam);
}

void sendAddedExamToServer(struct Exam exam) {
    int socketFD;
    struct sockaddr_in serverUniAddress;
    struct Request addExamRequest;

    addExamRequest.requestType = 1;
    addExamRequest.exam = exam;
    socketFD = connectToUniversita(socketFD, &serverUniAddress);

    //scrive su socketFD la struct addExamRequest che contiene l'esame da aggiungere
    if (write(socketFD, &addExamRequest, sizeof(addExamRequest)) != sizeof(addExamRequest)) {
        perror("Write error 7");
        exit(1);
    }
    printf("Esame aggiunto con successo!\n");

    //chiude la connsessione con il server
    close(socketFD);
}

int connectToUniversita(int socketFD, struct sockaddr_in *serverUniAddress) {
    //crea il socket per la comunicazione con il server università
    socketFD = Socket(AF_INET, SOCK_STREAM, 0);

    //inizializza la porta su cui è in ascolto il server e la converte in network order
    (*serverUniAddress).sin_family = AF_INET;
    (*serverUniAddress).sin_port = htons(1025);

    //converte l'indirizzo del server in un indirizzo di rete scritto in network order e lo salva in sin_addr
    if (inet_pton(AF_INET, "127.0.0.1", &(*serverUniAddress).sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", "127.0.0.1");
        exit(1);
    }

    //connette il socketFD all'indirizzo serverUniAddress
    Connect(socketFD, (struct sockaddr *) serverUniAddress, sizeof(*serverUniAddress));
    return socketFD;
}


