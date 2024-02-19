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
#include "wrapper.h"

struct Exam {
    char name[40];
    char date[25];
    short cfu;
} Exam;

struct Request {
    short requestType;
    struct Exam exam;
};

struct Booking {
    struct Exam exam;
    char matricola[11];
    int bookingId;
};

void addExamToFile(struct Exam);

int countExams();

void retriveExamList(struct Exam **);

void printExamsList(struct Exam *, int);

int selectExamsByName(struct Exam *, char [40], struct Exam **);

void sendSelectedExamsToSegreteria(int, struct Exam **, int);

void getExamAndMatricola(int connFD, struct Booking *bookingRequest);

void saveBookingOnFile(struct Booking *bookingRequest);

int main() {
    int listenFD, connFD;
    struct sockaddr_in serverAddress;
    struct Request recivedRequest;
    pid_t pid;

    //crea il socket listenFD per l'ascolto
    listenFD = Socket(AF_INET, SOCK_STREAM, 0);

    //definisce l'indirizzo del server e la porta sulla quale è in ascolto, con INADDR_ANY accetterà connessioni da qualsiasi indirizzo
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(1025);

    int enable = 1;
    setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    //assegna l'indirizzo serverAddress al socket listenFD
    Bind(listenFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

    //mette il socket in modalità di ascolto in attesa di nuove connessioni, con un massimo di 1025 client in attesa di essere accettati
    Listen(listenFD, 1025);
    while (1) {

        //accetta la connessione in ingresso
        connFD = Accept(listenFD, (struct sockaddr *) NULL, NULL);

        //quando arriva un nuovo client crea un processo figlio per gestire la connessione
        if ((pid = fork()) < 0) {
            perror("fork error");
            exit(-1);
        }

        //il processo figlio gestisce la connessione con il nuovo client
        if (pid == 0) {

            //chiude il socket in ascolto e interagisce con il client tramite connFD
            close(listenFD);

            //legge sul socket connFD la richiesta ricevuta dalla segreteria e la salva su recivedRequest
            if (read(connFD, &recivedRequest, sizeof(struct Request)) != sizeof(struct Request)) {
                perror("read error 1");
                exit(-1);
            }

            //se la richiesta è di tipo 1 allora l'esame viene aggiunto al file che contiente la lista delgi esami
            if (recivedRequest.requestType == 1) {
                addExamToFile(recivedRequest.exam);
            }
            // se la richiesta è di tipo 2 viene inviata alla segreteria la lista degli esami disponibili per il corso scelto dallo studente
            else if (recivedRequest.requestType == 2) {
                struct Exam *examsList;
                struct Exam *examsSelectedByName;
                int examsSelectedByNameCount;
                retriveExamList(&examsList);
                examsSelectedByNameCount = selectExamsByName(examsList, recivedRequest.exam.name, &examsSelectedByName);
                sendSelectedExamsToSegreteria(connFD, &examsSelectedByName, examsSelectedByNameCount);

                if (examsSelectedByNameCount > 0) {

                    struct Booking bookingRequest;
                    getExamAndMatricola(connFD, &bookingRequest);
                    bookingRequest.bookingId = 1;
                    saveBookingOnFile(&bookingRequest);

                    //iniva alla segreteria il numero di prenotazione progressivo assegnato allo studente
                    if (write(connFD, &bookingRequest.bookingId, sizeof(int)) != sizeof(int)) {
                        perror("write error 1");
                        exit(-1);
                    }
                }
                free(examsList);
                free(examsSelectedByName);

            }

            //chiude la connessione con il client e termina il processo figlio
            close(connFD);
            exit(0);
        } else {
            //il processo padre chiude il socket connFD e riprende l'ascolto su listenFD
            close(connFD);
        }
    }
    return 0;
}

void
saveBookingOnFile(struct Booking *bookingRequest) {//crea il file di prenotazione per l'esame con il nome dell'esame
    FILE *bookedExams;
    if ((bookedExams = fopen((*bookingRequest).exam.name, "a+")) == NULL) {
        printf("error opening the file");
        exit(1);
    }

    fseek(bookedExams, 0, SEEK_END);
    long filesize = ftell(bookedExams);

    if (filesize > 0) {
        struct Booking lastBooking;
        fseek(bookedExams, -sizeof(struct Booking), SEEK_END);
        fread(&lastBooking, sizeof(struct Booking), 1, bookedExams);
        (*bookingRequest).bookingId = lastBooking.bookingId + 1;
    }

    fwrite(bookingRequest, sizeof(struct Booking), 1, bookedExams);
    fclose(bookedExams);
}

void getExamAndMatricola(int connFD, struct Booking *bookingRequest) {

    //legge l'esame da prenotare
    if (read(connFD, &(*bookingRequest).exam, sizeof(struct Exam)) != sizeof(struct Exam)) {
        perror("read error 2");
        exit(-1);
    }

    //legge la matricola dello studente
    char matricola[11] = "0123456789";
    if (read(connFD, (*bookingRequest).matricola, sizeof(matricola)) != sizeof(matricola)) {
        perror("read error 3");
        exit(-1);
    }
}

void sendSelectedExamsToSegreteria(int connFD, struct Exam **examsSelectedByName, int examsSelectedByNameCount) {

    //invia alla segreteria il numero di esaemi selezionati
    if (write(connFD, &examsSelectedByNameCount, sizeof(int)) !=
        (sizeof(int))) {
        perror("Write error 2");
        exit(1);
    }

    //invia alla segreteria la lista di esami selezionati
    if (write(connFD, *examsSelectedByName, sizeof(struct Exam) * examsSelectedByNameCount) !=
        (sizeof(struct Exam) * examsSelectedByNameCount)) {
        perror("Write error 3");
        exit(1);
    }
}


//seleziona gli esami in base al nome examName
//alloca dinamicamente un array di struct formato
//salva sull'array di struct tutti gli esami che hanno lo stesso nome di examName
int selectExamsByName(struct Exam *examsList, char examName[40], struct Exam **examsSelectedByName) {
    int examsSelectedByNameCount = 0, j = 0, examsCount = countExams();
    for (int i = 0; i < examsCount; i++) {
        if ((strcmp(examsList[i].name, examName)) == 0) {
            examsSelectedByNameCount++;
        }
    }

    *examsSelectedByName = (struct Exam *) malloc(sizeof(struct Exam) * examsSelectedByNameCount);

    for (int i = 0; i < examsCount; i++) {
        if ((strcmp(examsList[i].name, examName)) == 0) {
            strcpy((*examsSelectedByName)[j].name, examsList[i].name);
            strcpy((*examsSelectedByName)[j].date, examsList[i].date);
            (*examsSelectedByName)[j].cfu = examsList[i].cfu;
            j++;
        }
    }

    printExamsList(*examsSelectedByName, examsSelectedByNameCount);
    return examsSelectedByNameCount;
}

void printExamsList(struct Exam *examsList, int examsCount) {
    for (int i = 0; i < examsCount; i++) {
        printf("%s\n", examsList[i].name);
        printf("%s\n", examsList[i].date);
        printf("%hd\n", examsList[i].cfu);
    }
}

//recupera tutti gli esami salvati sul file examList
//alloca dinamicamente un array di struct examsList
//salva su examsList tutti gli esami
void retriveExamList(struct Exam **examsList) {
    struct Exam readedExam;
    int examsCounter = 0, examsCount = countExams();

    FILE *examsListFD;
    if ((examsListFD = fopen("examList", "r+")) == NULL) {
        printf("error opening the file");
        exit(1);
    }

    *examsList = (struct Exam *) malloc(sizeof(struct Exam) * examsCount);
    while (!feof(examsListFD)) {
        fread(&readedExam, sizeof(struct Exam), 1, examsListFD);
        strcpy((*examsList)[examsCounter].name, readedExam.name);
        strcpy((*examsList)[examsCounter].date, readedExam.date);
        (*examsList)[examsCounter].cfu = readedExam.cfu;
        examsCounter++;
    }

    //chiudendo il file il programma va in errore (double free or corruption (!prev))
//    fclose(examsListFD);
//    printExamsList(*examsList, examsCount);
}

//conta quanti esami sono salvati sul file examList
int countExams() {
    int i = 0;
    FILE *examsListFD;

    if ((examsListFD = fopen("examList", "r")) == NULL) {
        printf("error opening the file");
        exit(1);
    }

    fseek(examsListFD, 0, SEEK_END);
    long filesize = ftell(examsListFD);
    i = filesize / sizeof(struct Exam);
    fclose(examsListFD);

    return i;
}

//aggiunge l'esame exam in coda al file examList
void addExamToFile(struct Exam exam) {
    FILE *examListFD;
    if ((examListFD = fopen("examList", "a+")) == NULL) {
        printf("error opening the file");
        exit(1);
    }

    fwrite(&exam, sizeof(struct Exam), 1, examListFD);
    fclose(examListFD);
    printf("Esame aggiunto con successo!\n");
}


