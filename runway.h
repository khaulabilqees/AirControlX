
#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "flight.h"

class RunwayManager {
public:
    pthread_mutex_t locks[3]; // 3 diff runways so only one thread can have access at a time for one 
    bool availability[3]; // to see runways are available or not currently

    RunwayManager() {
        for (int i = 0; i < 3; ++i)
            pthread_mutex_init(&locks[i], NULL);
    }

    int assignRunway(Flight* f) {
        if (f->typeString == "emmergency" && (availability[0] || availability[1]) && (availability[3] == 0) )
            return RUNWAY_C ; // if A and B runways are not available for high priority flights then runway C should be taken if available
        if ((f->direction == 0 || f->direction == 1) && f->type != TYPE_CARGO) 
            return RUNWAY_A;
        if ((f->direction == 2 || f->direction == 3) && f->type != TYPE_CARGO) 
            return RUNWAY_B;
        return RUNWAY_C;
    }

    void lockRunway(int r) { 
        pthread_mutex_lock(&locks[r]); 
        availability[r] = 1;
    }
    void unlockRunway(int r) { pthread_mutex_unlock(&locks[r]); 
        availability[r] = 0;
    }

    ~RunwayManager() {
        for (int i = 0; i < 3; ++i)
            pthread_mutex_destroy(&locks[i]);
    }
};
