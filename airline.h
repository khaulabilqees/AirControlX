#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "flight.h"

int currId; 

class AVN{ // avn generator
public: 
    int avnID;
    int flightID;
    char airline[50];
    int type;
    char reason[50];
    int recordedSpeed;
    int permittedSpeed;
    int fineAmount;
    int time; 
    bool paid;

    AVN(const AVN& other) {
        avnID = other.avnID;
        flightID = other.flightID;
        strcpy(airline, other.airline);
        type = other.type;
        strcpy(reason, other.reason);
        recordedSpeed = other.recordedSpeed;
        permittedSpeed = other.permittedSpeed;
        fineAmount = other.fineAmount;
        time = other.time;
        paid = other.paid;
    }

    AVN(){}
    
};


class Airline {
public:
    string name;
    string type; // type of airline
    vector<Flight*> flights;
    int numFlights;
    int numAircrafts;
    int numViolations; // keep track of how many aircrafts in violation
    vector<AVN*> portal ; // every airline has its own airline portal

    Airline(string n, string t) : name(n), type(t) {
    }

    void addFlight(int flightID, string type, int direction, int schT ) { // type of flight is being sent here 
        flights.push_back(new Flight(flightID, name, type, direction, schT));
    }

    void addAvn(AVN* a){
        portal.push_back(a);
    }
    void CreateAirline(int numfl, int numAir) {
        cout << "Creating AirLine: " << name << " of type: " << type << endl;
        numFlights = numfl;
        numAircrafts = numAir;
        for (int i=0; i < numFlights ; i++){
            string directions[] = {"north", "east" , "west", "south "};
            int id = totalCount++;
            string tipe;
            int schT = rand()%10 ;
            if (type == "commercial")
                tipe = "commercial";
            else if (type == "cargo")
                tipe = "cargo";
            else if(type == "emergency" || type == "medical" || type == "military" )
                tipe = "emergency";
            
            int dir = rand()% 4;
            addFlight(id, tipe, dir, schT);
        }

    } 
    ~Airline() {
        for (int i = 0; i < flights.size(); i++) {
            delete flights[i];
        }
    }
};
