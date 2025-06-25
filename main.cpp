#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <string.h>
#include <ctime>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "acts.h"

using namespace std;

volatile sig_atomic_t stopFlag = 0;
void handle_sigterm(int sig) {
    stopFlag = 1;
}

vector<AVN*> allAVNs;
AVN* temp;

void avnController() {
    signal(SIGTERM, handle_sigterm);
    while (!stopFlag) {
        ViolationRecord v;
        int fd = open("violationPipe", O_RDONLY);
        int bytes = read(fd, &v, sizeof(ViolationRecord));
        close(fd);

        if (bytes > 0) {
            AVN avn;
            avn.avnID = ++currId;
            avn.flightID = v.flightID;
            memcpy(avn.airline, v.airline, sizeof(v.airline));
            strcpy(avn.reason, "speed exceeded\n");
            avn.type = v.type;
            avn.recordedSpeed = v.recordedSpeed;
            avn.permittedSpeed = v.permittedSpeed;
            avn.paid = false;
            avn.time = v.timestamp;
            avn.fineAmount = (v.type == 1 ? 500000 : 700000);
            avn.fineAmount += (0.15 * avn.fineAmount);
            temp = new AVN(avn);
            allAVNs.push_back(temp);

            cout << "[AVN Generator] Generated AVN " << avn.avnID << " for Flight " << avn.flightID << " at time: " << avn.time << endl;

            fd = open("avnResultsPipe", O_WRONLY);
            write(fd, &avn, sizeof(AVN));
            close(fd);
        }
    }
    exit(0);
}

void airlinePortalController() {
    signal(SIGTERM, handle_sigterm);
    while (!stopFlag) {
        AVN avn;
        int fd = open("paymentToAirlinePortal", O_RDONLY | O_NONBLOCK);
        int bytes = read(fd, &avn, sizeof(AVN));
        close(fd);

        if (bytes > 0) {
            cout << "[Airline Portal] AVN ID: " << avn.avnID
                 << " for Flight " << avn.flightID
                 << " has been " << (avn.paid ? "paid.\n" : "unpaid.\n");
        }
    }
    exit(0);
}

void stripePayProcess() {
    signal(SIGTERM, handle_sigterm);
    while (!stopFlag) {
        AVN avn;
        int fd = open("AirlineToPayment", O_RDONLY);
        int bytes = read(fd, &avn, sizeof(AVN));
        close(fd);

        if (bytes > 0) {
            cout << "\n[StripePay] Payment Request for AVN ID: " << avn.avnID
                 << " | Flight ID: " << avn.flightID
                 << " | Aircraft Type: " << (avn.type == 1 ? "commercial\n" : "cargo\n")
                 << " | Fine: " << avn.fineAmount << endl;

            cout << "[StripePay] Payment Successful\n";

            fd = open("paymentToAirlinePortal", O_WRONLY | O_NONBLOCK);
            write(fd, &avn, sizeof(AVN));
            close(fd);
        }
    }
    exit(0);
}

void* readAVNsFromPipe(void* arg) {
    while (!stopFlag) {
        AVN avn;
        int fd = open("avnResultsPipe", O_RDONLY);
        int bytes = read(fd, &avn, sizeof(AVN));
        close(fd);

        if (bytes > 0) {
            AVN* newAvn = new AVN(avn);
            allAVNs.push_back(newAvn);
            cout << "[Parent] Received AVN " << avn.avnID << " from pipe\n";
        }
    }
    return nullptr;
}

int main() {
    mkfifo("paymentToAVNGen", 0666);
    mkfifo("paymentToAirlinePortal", 0666);
    mkfifo("avnResultsPipe", 0666);
    mkfifo("violationPipe", 0666);
    mkfifo("avnPipe", 0666);
    mkfifo("avnToAirline", 0666);
    mkfifo("AirlineToPayment", 0666);

    pid_t pid1 = fork();
    if (pid1 == 0) avnController();

    pid_t pid3 = fork();
    if (pid3 == 0) stripePayProcess();

    pid_t pid4 = fork();
    if (pid4 == 0) airlinePortalController();

    pthread_t avnReaderThread;
    pthread_create(&avnReaderThread, NULL, readAVNsFromPipe, NULL);

    ATCS system;
    system.createAirlines();
    system.displayAllAirlinesAndFlights();
    system.simulateAllFlights();

    for (int i = 0; i < allAVNs.size(); i++) {
        cout << "AVN ID: " << allAVNs[i]->avnID
             << ", Airline: " << allAVNs[i]->airline
             << ", Flight: " << allAVNs[i]->flightID
             << ", Fine: " << allAVNs[i]->fineAmount
             << ", Paid: " << (allAVNs[i]->paid ? "Yes" : "No") << endl;
    }

    for (int i = 0; i < allAVNs.size(); i++) {
        for (int j = 0; j < system.airlines.size(); j++) {
            if (system.airlines[j]->name == allAVNs[i]->airline) {
                system.airlines[j]->addAvn(allAVNs[i]);
                break;
            }
        }
    }

    for (int i = 0; i < system.airlines.size(); i++) {
        cout << "\nAVNs for Airline: " << system.airlines[i]->name << endl << endl;
        for (int j = 0; j < system.airlines[i]->portal.size(); j++) {
            cout << "AVN ID: " << system.airlines[i]->portal[j]->avnID
                 << ", Fine: " << system.airlines[i]->portal[j]->fineAmount << "\nPay now? (y/n): ";
            char c;
            cin >> c;
            if (c == 'y' || c == 'Y') {
                system.airlines[i]->portal[j]->paid = true;
                int fd = open("AirlineToPayment", O_WRONLY);
                write(fd, system.airlines[i]->portal[j], sizeof(AVN));
                close(fd);
                sleep(2);
            }
        }
    }

    // Cleanup
    stopFlag = 1;
    pthread_cancel(avnReaderThread);
    pthread_join(avnReaderThread, NULL);

    kill(pid1, SIGTERM);
    kill(pid3, SIGTERM);
    kill(pid4, SIGTERM);

    waitpid(pid1, NULL, 0);
    waitpid(pid3, NULL, 0);
    waitpid(pid4, NULL, 0);

    unlink("paymentToAVNGen");
    unlink("paymentToAirlinePortal");
    unlink("avnResultsPipe");
    unlink("violationPipe");
    unlink("avnPipe");
    unlink("avnToAirline");
    unlink("AirlineToPayment");

    return 0;
}
