#ifndef FLIGHT_H
#define FLIGHT_H

#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include<sys/stat.h>
#include<sys/wait.h>
#include <SFML/System.hpp>

using namespace std;

#define RUNWAY_A 0
#define RUNWAY_B 1
#define RUNWAY_C 2

#define HOLD_MIN 400
#define HOLD_MAX 600
#define APPROACH_MIN 240
#define APPROACH_MAX 290
#define LANDING_END 30
#define TAXI_MAX 30
#define AT_GATE_MAX 5
#define TAKEOFF_MAX 290
#define CLIMB_MAX 463
#define CRUISE_MIN 800
#define CRUISE_MAX 900

#define TYPE_COMMERCIAL 1
#define TYPE_CARGO 2
#define TYPE_EMERGENCY 3

sf::Clock simClock;

int flightToAirline[2];


int totalCount = 1 ;
const int simulationDuration = 50 ; // in seconds
time_t simulationStartTime;
pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;  

struct ViolationRecord {
    int flightID;
    char airline[50];
    int recordedSpeed;
    int permittedSpeed;
    int type;
    int timestamp;
};

class Flight {
public:
    int flightID;
    string airline;
    int type; 
    string typeString;
    int direction;
    string status;
    int speed;
    bool AVNActive;
    bool groundFault;
    int runway;
    string runwayStr; 
    int priority;
    bool violation;
    int scheduledTime;
    int actualTime ; // wait time will be its scheduled time minus the time its simulate flight func is call 
                    // so the time it spends in the queue waiting AFTER its scheduled time.
    ViolationRecord record; 

    Flight(int id, string al, string t, int dir, int schTime) : flightID(id), airline(al), typeString(t), direction(dir), AVNActive(false),
        groundFault(false), status("Queued"), speed(0), runway(-1), priority(0), violation(false), scheduledTime(schTime) {
            if (typeString == "commercial"){
                type = 1;
                priority = 3;
            }
            else if (typeString == "cargo"){
                type = 2;
                priority = 2;
            }
            else if (typeString == "emergency"){
                type = 3;
                priority = 1;
            }

    }

     int timestamp() const {
        return static_cast<int>(simClock.getElapsedTime().asSeconds());
    }
    void simulate() {

        actualTime = timestamp();
        int waitSeconds = actualTime - scheduledTime;

        pthread_mutex_lock(&coutMutex);
        cout << "[WAIT] Flight " << flightID << " (" << airline << ") waited " << waitSeconds << " seconds before execution.\n";
        pthread_mutex_unlock(&coutMutex);

        

        if (direction == 0 || direction == 1) { // arrival (north / south)
            status = "Holding";
            int offset = rand()%11;
            //speed = rand() % (HOLD_MAX - HOLD_MIN + 1) + HOLD_MIN;
            speed = max(0, HOLD_MIN + rand() % (HOLD_MAX - HOLD_MIN + 1) + offset);
            printStatus(timestamp());
            checkViolation(HOLD_MAX, HOLD_MIN);
            //sleep(1);
            sf::sleep(sf::seconds(1));

            status = "Approach";
            //speed = rand() % (APPROACH_MAX - APPROACH_MIN + 1) + APPROACH_MIN;
            offset = rand()%11;
            speed = max(0, APPROACH_MIN + rand() % (APPROACH_MAX - APPROACH_MIN + 1) + offset);
            printStatus(timestamp());
            checkViolation(APPROACH_MAX, APPROACH_MIN);
            //sleep(1);
            sf::sleep(sf::seconds(1));

            status = "Landing";
            speed = 240;
            printStatus(timestamp());
            for (int i = 0; i < 3; i++) {
                speed -= 70;
                if (speed <= LANDING_END) 
                    break;
                //sleep(1);
                sf::sleep(sf::seconds(1));
            }
            checkViolation(APPROACH_MAX, LANDING_END);


            status = "Taxi";
            speed = rand() % 16 + 15;
            printStatus(timestamp());
            checkViolation(TAXI_MAX, 15);
            //sleep(1);
            sf::sleep(sf::seconds(1));

            status = "At Gate";
            offset = rand()%6;
            speed = 0 + offset;
            printStatus(timestamp());
            checkViolation(AT_GATE_MAX, 0);

        } else { // when departure (direction east west)
            status = "At Gate";
            int offset = rand()%8;
            speed = 0 + offset;
            printStatus(timestamp());
            checkViolation(AT_GATE_MAX, 0);
            //sleep(1);
            sf::sleep(sf::seconds(1));

            status = "Taxi";
            speed = rand() % 16 + 15;
            printStatus(timestamp());
            checkViolation(TAXI_MAX, 15);
            //sleep(1);
            sf::sleep(sf::seconds(1));

            status = "Takeoff Roll";
            speed = 0;
            printStatus(timestamp());
            for (int i = 0; i < 3; i++) {
                speed += 100;
                if (speed > TAKEOFF_MAX) {
                    violation = true;
                    break;
                }
                //sleep(1);
                sf::sleep(sf::seconds(1));
            }

            status = "Climb";
            //speed = rand() % (CLIMB_MAX - 250 + 1) + 250;
            offset = rand()%11;
            speed = max(0, 250 + rand() % (CLIMB_MAX - 250 + 1) + offset);
            printStatus(timestamp());
            checkViolation(CLIMB_MAX, 250);
            //sleep(1);
            sf::sleep(sf::seconds(1));

            status = "Cruise";
            speed = rand() % (CRUISE_MAX - CRUISE_MIN + 1) + CRUISE_MIN;
            //cout << "[" << flightID << "] " << status << " | Speed: " << speed << " km/h\n";
            printStatus(timestamp());
            checkViolation(CRUISE_MAX, CRUISE_MIN);
        }
    }

    void printStatus(int simTime) {
        pthread_mutex_lock(&coutMutex);
        cout << "[TIME: " << simTime << "s] Flight " << flightID << " (" << airline << ") | Status: " << status << " | Speed: " << speed << " km/h\n";
        pthread_mutex_unlock(&coutMutex);
    }

private:
    bool checkViolation(int max, int min) {
        if (speed < min || speed > max){
            violation = true;
            AVNActive = true;
            pthread_mutex_lock(&coutMutex);
            cout << "[VIOLATION] Flight " << flightID << ": Speed violation in phase " << status << "\n";
            pthread_mutex_unlock(&coutMutex);

            /*string violationMsg = to_string(flightID) + "|" + airline + "|" + typeString + "|" + status + "|" +
                              to_string(speed) + "|" + to_string(min) + "-" + to_string(max) + "|" +
                              to_string(timestamp()) + "\n";*/
            

            
            int fd = open("violationPipe", O_WRONLY | O_NONBLOCK);
            ViolationRecord rec;
            rec.flightID = flightID;
            strcpy(rec.airline, airline.c_str());
            rec.recordedSpeed =speed;
            rec.permittedSpeed= max;
            rec.timestamp = timestamp();
            rec.type = type ;
            write(fd, &rec, sizeof(rec));
            close(fd);
            //cout << "violation of " << flightID << "sent" << endl ;
            AVNActive = false;
            violation = false; 
            
            return true ;
        }

        return false;  
        
    }
};


#endif