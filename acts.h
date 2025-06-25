#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "airline.h"
#include "runway.h"
#include <SFML/System.hpp> 

struct ThreadArgs {
    Flight* flight;
    RunwayManager* manager;

    ThreadArgs(Flight* fl , RunwayManager* rm){
        flight = fl;
        manager = rm;
    }
};

void* flightThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    Flight* f = args->flight;
    RunwayManager* rm = args->manager;

    f->runway = rm->assignRunway(f);
    /* if (f->runway = 0)
        f->runwayStr = "RUNWAY-A";
    else if (f->runway = 1)
        f->runwayStr = "RUNWAY-B";
    else 
        f->runwayStr = "RUNWAY-C"; */

    rm->lockRunway(f->runway);
    pthread_mutex_lock(&coutMutex);
    cout << "[Assigned] " << f->flightID << " to RWY-" << char('A' + f->runway) << endl;
    pthread_mutex_unlock(&coutMutex);
    f->simulate();
    sleep(1);
    pthread_mutex_lock(&coutMutex);
    cout << "INFO: Flight " << f->flightID << " completed all phases.\n";
    //cout << f->flightID << " released " << f->runwayStr << endl;
    pthread_mutex_unlock(&coutMutex);
    rm->unlockRunway(f->runway);

    //delete args;
    pthread_exit(NULL);
}

class ATCS { 
public:
    vector<Airline*> airlines;
    RunwayManager rm;
    vector<Flight*> arrivalQueue; // no need for these initially put all in one queue and then based on scheduled time put in arrival and departure qeuues
    vector<Flight*> departureQueue; // same as above
    vector<Flight*> activeArrival;
    vector<Flight*> activeDeparture;
    vector<Flight*> allFlights; 
    //pthread_mutex_t coutMutex;

    // function to add airline 
    /*void addAirline(string name, string type, vector<pair<string, int>> flightInfo) {
        Airline* a = new Airline(name, type);
        for (int i=0 ; i<flightInfo.size() ; i++)
            a->addFlight(flightInfo[i].first, flightInfo[i].second);
        airlines.push_back(a);
    }*/

    void createAirlines(){
        srand(time(0));
        Airline* a1 = new Airline("PIA", "commercial");
        a1->CreateAirline(4, 6);
        airlines.push_back(a1);

        Airline* a2 = new Airline("FedEx", "cargo");
        a2->CreateAirline(2, 3);
        airlines.push_back(a2);

        Airline* a3 = new Airline("PAF", "military");
        a3->CreateAirline(1, 2);
        airlines.push_back(a3);

        Airline* a4 = new Airline("AirBlue", "commercial");
        a4->CreateAirline(4, 4);
        airlines.push_back(a4);

        Airline* a5 = new Airline("BlueDart", "cargo");
        a5->CreateAirline(2, 2);
        airlines.push_back(a5);

        Airline* a6 = new Airline("AghaKhan Air", "medical");
        a6->CreateAirline(4, 4);
        airlines.push_back(a6);

        //coutMutex = PTHREAD_MUTEX_INITIALIZER;
    }

    static bool flightPriorityCompare(Flight* a, Flight*b){
        return ( a->priority < b->priority ) ;
    }

    void printQueue(const string& label, const vector<Flight*>& queue) {
    cout << "\n" << label << ":\n";
    for (int i = 0; i < queue.size(); i++) {
        cout << "  Flight ID: " << queue[i]->flightID
             << " | Airline: " << queue[i]->airline
             << " | Type: " << queue[i]->typeString
             << " | Direction: " << queue[i]->direction
             << " | Priority: " << queue[i]->priority
            // << " | Enqueued at: " << queue[i]->enqueueTime
             << endl;
        }
    }

    void launchQueue(vector<Flight*>& queue, vector<pthread_t>& threads) {
        //cout << queue.size() << endl ;
        for (int i = 0; i < queue.size(); i++) {

            Flight* f = queue[i];
            pthread_t tid;
            ThreadArgs* args = new ThreadArgs(f, &rm);
            if (pthread_create(&tid, NULL, flightThread, args) == 0) {
                threads.push_back(tid);
                // Remove flight from queue after thread is created successfully
                queue.erase(queue.begin() + i);
            }         
        }
    }
    

    void simulateAllFlights() {
        vector<pthread_t> threads;
        for (int i=0 ; i<airlines.size() ; i++) {
            for (int j= 0 ;  j < airlines[i]->flights.size() ; j++) {
                allFlights.push_back(airlines[i]->flights[j]); // all flights are in one place now 
            }
        }
        while (simClock.getElapsedTime().asSeconds() <= simulationDuration) {
            int currTime = static_cast<int>(simClock.getElapsedTime().asSeconds());
            pthread_mutex_lock(&coutMutex);
            //cout << "\n=== SIM TIME: " << currTime << "s ===\n";
            pthread_mutex_unlock(&coutMutex);

            

            for (int i= 0 ; i<allFlights.size() ; i++ ) {
                if ((allFlights[i]->scheduledTime == currTime) && (allFlights[i]->direction == 0 || allFlights[i]->direction == 1 )){
                    activeArrival.push_back(allFlights[i]);
                    
                }
                    
                else if ((allFlights[i]->scheduledTime == currTime) && (allFlights[i]->direction == 2 || allFlights[i]->direction == 3 )){
                    activeDeparture.push_back(allFlights[i]);
                    //cout << "boo\n";
                }
                //cout << "boo\n";
                    
            }

            sort(activeArrival.begin(), activeArrival.end(), ATCS::flightPriorityCompare);
            sort(activeDeparture.begin(), activeDeparture.end(), ATCS::flightPriorityCompare);
            //cout << "boo2\n";
            launchQueue(activeArrival, threads);
            launchQueue(activeDeparture, threads);
            /* for (Flight* f : toLaunch) {
                ThreadArgs* args = new ThreadArgs(f, &rm);
                pthread_t tid;
                pthread_create(&tid, NULL, flightThread, args);
                activeThreads.push_back(tid);

                if (f->direction == 0 || f->direction == 1)
                    arrivalQueue.erase(remove(arrivalQueue.begin(), arrivalQueue.end(), f), arrivalQueue.end());
                else
                    departureQueue.erase(remove(departureQueue.begin(), departureQueue.end(), f), departureQueue.end());
            } */

            sf::sleep(sf::seconds(1));
        }

        for (int i=0 ; i< threads.size() ; i++)
            pthread_join(threads[i], NULL); 
    }

    void displayAllAirlinesAndFlights() {
    cout << "\n=== Airline and Flight Information ===\n";
        for (int i = 0; i < airlines.size(); i++) {
            Airline* airline = airlines[i];
            cout << "Airline: " << airline->name << " (" << airline->type << ")\n";
            for (int j = 0; j < airline->flights.size(); j++) {
                Flight* flight = airline->flights[j];
                cout << "  Flight ID: " << flight->flightID
                    << " | Type: " << flight->typeString
                    << " | Direction: " << flight->direction
                    << " | Status: " << flight->status
                    << " | Priority: " << flight->priority
                    << " | schedule time: " << flight->scheduledTime
                    << "\n";
            }
            cout << endl;
        }
    }

    ~ATCS() {
        for (int i =0 ; i< airlines.size() ;i++)
            delete airlines[i];
    }
};
