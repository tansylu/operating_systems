#include <iostream>
#include <pthread.h>
#include <vector>

using namespace std;

// Struct to represent a fan
struct Fan {
    int teamID;      // 1 for Team A, 2 for Team B
    bool foundSpot;  // Flag to indicate whether the fan found a spot in the car
    bool isCaptain;  // Flag to identify the captain
};

// Struct to represent a car
struct Car {
    int carID;        // Car ID
    Fan captain;      // Captain of the car
    int chosen_A;     // Count of fans from Team A in the car
    int chosen_B;     // Count of fans from Team B in the car
};

// Vectors to keep track of fans from each club
vector<Fan> fansA;
vector<Fan> fansB;

// Mutex and condition variables for synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Counter for fans from each team
int fansATotal = 0;
int fansBTotal = 0;

// Total barriers needed for cars
int totalBarriers;

// Car ID counter
int carIDCounter = 0;

// Vector to store cars
vector<Car*> cars;

// Barriers for each car
pthread_barrier_t* carBarriers;


// Function to check if a fan can be added to a car
bool canAddToCar(int& fansATotal, int& fansBTotal) {
    // Check if the car can be filled with 2 of team A + 2 of team B
    if (fansATotal >= 2 && fansBTotal >= 2) {
        return true;
    }
    // Check if the car can be filled with 4 of team A
    else if (fansATotal >= 4) {
        return true;
    }
    // Check if the car can be filled with 4 of team B
    else if (fansBTotal >= 4) {
        return true;
    }
    return false;
}

// Thread function for fans
void* fanThread(void* arg) {
    Fan* fan = static_cast<Fan*>(arg);

    // Increment the counters based on the team
    pthread_mutex_lock(&mutex);
    int currentCarID = carIDCounter;

    cout << "Thread ID: " << pthread_self() << ", Team: " << (fan->teamID == 1 ? 'A' : 'B')
             << ", I am looking for a car no " << currentCarID << endl;

    if (fan->teamID == 1) {
        fansATotal++;
    } else {
        fansBTotal++;
    }
    pthread_mutex_unlock(&mutex);

    bool foundSpot = false;

    while (!foundSpot) {
        pthread_barrier_wait(&carBarriers[currentCarID]);
        pthread_mutex_lock(&mutex);
        bool flag = canAddToCar(fansATotal, fansBTotal);

        if (flag) {
            if (currentCarID + 1 > cars.size()) {
                fan->isCaptain = true;
                Car* curr = new Car;
                cars.push_back(curr);
                cars[currentCarID]->captain = *fan;
                cars[currentCarID]->carID = currentCarID;
                cars[currentCarID]->chosen_A = fan->teamID == 1 ? 1 : 0;
                cars[currentCarID]->chosen_B = fan->teamID == 1 ? 0 : 1;

                cout << "Thread ID: " << pthread_self() << ", Team: " << (fan->teamID == 1 ? 'A' : 'B')
                     << ", I am the captain of the car (Car ID: " << (currentCarID) << ")." << endl;
                carIDCounter++;
                foundSpot = true;  // Exit the loop, the car is formed
                pthread_mutex_unlock(&mutex);
            } 
            else if (cars[currentCarID]->chosen_A + cars[currentCarID]->chosen_B < 4) {
                if (fan->teamID == 1) {
                    cars[currentCarID]->chosen_A += 1;
                } else {
                    cars[currentCarID]->chosen_B += 1;
                }
            
                cout << "Thread ID: " << pthread_self() << ", Team: " << (fan->teamID == 1 ? 'A' : 'B')
                     << ", I found a spot in the car (Car ID: " << (carIDCounter - 1) << ")." << endl;
                
                foundSpot = true;  // Exit the loop, the fan found a spot in the car
                pthread_mutex_unlock(&mutex);
            } 
            else{
                    cout << "car id:"<<currentCarID<<"is filled and left"<<endl;
                    fansATotal -= cars[currentCarID]->chosen_A;
                    fansBTotal -= cars[currentCarID]->chosen_B;
                    currentCarID++;
                     pthread_mutex_unlock(&mutex);
            }
        } else {
            currentCarID++;
             pthread_mutex_unlock(&mutex);
        }

    }

    return nullptr;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Please enter 2 numbers";
        return 1;
    }

    int ASize = stoi(argv[1]);
    int BSize = stoi(argv[2]);

    if (ASize % 2 != 0 || BSize % 2 != 0 || (ASize + BSize) % 4 != 0) {
        cerr << "Invalid number of fans. Each group size must be even, and the total number must be a multiple of four.\n";
        return 1;
    }

    pthread_t threads[ASize + BSize];

    fansA.resize(ASize);
    fansB.resize(BSize);

    totalBarriers = (ASize + BSize) / 4;

    // Initialize barriers for each car
    carBarriers = new pthread_barrier_t[totalBarriers];
    for (int i = 0; i < totalBarriers; ++i) {
        pthread_barrier_init(&carBarriers[i], nullptr, 4);
    }

    // Initialize fans for Team A
    for (int i = 0; i < ASize; ++i) {
        fansA[i].teamID = 1;  // Team A
        fansA[i].foundSpot = false;
        fansA[i].isCaptain = false;
    }

    // Initialize fans for Team B
    for (int i = 0; i < BSize; ++i) {
        fansB[i].teamID = 2;  // Team B
        fansB[i].foundSpot = false;
        fansB[i].isCaptain = false;
    }

    // Create threads and pass initialized fans
    for (int i = 0; i < ASize + BSize; ++i) {
        pthread_create(&threads[i], nullptr, fanThread, (i < ASize) ? &fansA[i] : &fansB[i]);
    }

    for (int i = 0; i < ASize + BSize; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Destroy barriers
    for (int i = 0; i < totalBarriers; ++i) {
        pthread_barrier_destroy(&carBarriers[i]);
    }

    delete[] carBarriers;

    cout << "The main thread terminates.\n";

    return 0;
}