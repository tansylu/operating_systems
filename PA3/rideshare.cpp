#include <iostream>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for protecting output
pthread_cond_t departure_cond = PTHREAD_COND_INITIALIZER;
int type_a_count = 0;
int type_b_count = 0;
int car_count = 0;

bool car_in_progress = false;  // Flag indicating whether a car is being filled and departed

// Semaphores for controlling the number of type A and type B threads
sem_t sem_a;
sem_t sem_b;

// Car struct
struct Car {
    bool empty;
    pthread_t captain_thread_id;
    int type_a_count;  // Count of type A threads in the car
    int type_b_count;  // Count of type B threads in the car

    Car() : empty(true), captain_thread_id(-1), type_a_count(0), type_b_count(0) {}
};

// Vector of cars
vector<Car> cars;

// Vector of threads
vector<char> threads;

void debug_print(const string& message) {
    pthread_mutex_lock(&output_mutex);
   cout<<message << endl;
    pthread_mutex_unlock(&output_mutex);
}

void safe_cout(const string& message) {
    pthread_mutex_lock(&output_mutex);
    cout << message << endl;
    pthread_mutex_unlock(&output_mutex);
}

// Function to initialize cars
void initializeCars(int num_cars) {
    for (int i = 0; i < num_cars; ++i) {
        cars.push_back(Car());
    }
}

void* fan_func(void* arg) {
    char thread_type = *static_cast<char*>(arg);
    pthread_mutex_lock(&mutex);

    // Check if a car is in progress
    while (car_in_progress) {
        pthread_cond_wait(&departure_cond, &mutex);
    }

    if (thread_type == 'A') {
        type_a_count++;
    } else {
        type_b_count++;
    }

    bool condition_1 = (type_a_count >= 4 || type_b_count >= 4);
    bool condition_2 = (type_a_count >= 2 && type_b_count >= 2);
    if (condition_1 || condition_2) {
        debug_print("Thread ID: " + to_string(pthread_self()) + ", Team: " + thread_type + ", I am looking for a car");
        car_in_progress = true;  // Set car in progress

        // First one to reach
        if (thread_type == 'A') {
            cars[car_count].type_a_count++;
        } else {
            cars[car_count].type_b_count++;
        }
        debug_print("Thread ID: " + to_string(pthread_self()) + ", Team: " + thread_type + ", I have found a spot in a car");
        
        if (thread_type == 'A') {
                sem_post(&sem_a);       
            }
        else {
                sem_post(&sem_b); 
              }
        
       pthread_mutex_unlock(&mutex);
    } else {
        debug_print("Thread ID: " + to_string(pthread_self()) + ", Team: " + thread_type + ", I am looking for a car");
        pthread_mutex_unlock(&mutex);
        sem_wait((thread_type == 'A') ? &sem_a : &sem_b);  // Wait for the required number of threads
        pthread_mutex_lock(&mutex);
        bool condition_1 = (type_a_count >= 4 || type_b_count >= 4);
        bool condition_2 = (type_a_count >= 2 && type_b_count >= 2);
        int current_car = car_count;
        if (thread_type == 'A') {
            cars[current_car].type_a_count++;
        } else {
            cars[current_car].type_b_count++;
        }

        debug_print("Thread ID: " + to_string(pthread_self()) + ", Team: " + thread_type + ", I have found a spot in a car");

        if (cars[current_car].type_a_count + cars[current_car].type_b_count == 4 &&cars[current_car].captain_thread_id == -1 ) {
            // After adding myself, it became full
            cars[current_car].captain_thread_id = pthread_self();  // The last thread becomes captain
            }
        if (cars[current_car].type_a_count + cars[current_car].type_b_count == 4 &&cars[current_car].captain_thread_id == pthread_self()) {
            type_a_count -= cars[current_car].type_a_count;        // Update globals
            type_b_count -= cars[current_car].type_b_count;
            debug_print("Thread ID: " + to_string(pthread_self()) + ", Team: " + thread_type + ", I am the captain and driving the car with ID " + to_string(current_car));
            car_in_progress = false;  // Reset car in progress when the car departs
            car_count++;
            current_car = car_count;
            pthread_cond_broadcast(&departure_cond);
        }
        else{
            if(condition_1){
                 if (thread_type == 'A') {
                sem_post(&sem_a);  
            }
            else {
                sem_post(&sem_b); 
              }
            }
            else if(condition_2){
                if(cars[current_car].type_b_count <2){
                     sem_post(&sem_b); 
                }
                else if(cars[current_car].type_a_count < 2){
                     sem_post(&sem_a); 
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);

    return nullptr;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Please enter 2 numbers";
        return 1;
    }

    int num_threads_a = stoi(argv[1]);
    int num_threads_b = stoi(argv[2]);

    if (num_threads_a % 2 != 0 || num_threads_b % 2 != 0 || (num_threads_a + num_threads_b) % 4 != 0) {
        cerr << "The main terminates.\n";
        return 1;
    }

    // Pre-init cars
    int initial_car_capacity = (num_threads_a + num_threads_b) / 4;
    initializeCars(initial_car_capacity);

    // Initialize semaphores
    sem_init(&sem_a, 0, 0);
    sem_init(&sem_b, 0, 0);

    pthread_t pthreads[num_threads_a + num_threads_b];
    threads.reserve(num_threads_a + num_threads_b);

    // Create threads for type A
    for (int i = 0; i < num_threads_a; ++i) {
        threads.push_back('A');
        pthread_create(&pthreads[i], nullptr, fan_func, &threads.back());
    }

    // Create threads for type B
    for (int i = num_threads_a; i < num_threads_a + num_threads_b; ++i) {
        threads.push_back('B');
        pthread_create(&pthreads[i], nullptr, fan_func, &threads.back());
    }

    // Join threads
    for (int i = 0; i < num_threads_a + num_threads_b; ++i) {
        pthread_join(pthreads[i], nullptr);
    }

    cout <<"The main terminates"<<endl;
    return 0;
}
