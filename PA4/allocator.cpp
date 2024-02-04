#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

pthread_mutex_t printMutex;

using namespace std;
class HeapManager {
private:
    pthread_mutex_t mtx;


    struct Node {
        int ID;    
        int SIZE;  
        int INDEX; 
        Node* next; 

        Node(int id, int size, int index) : ID(id), SIZE(size), INDEX(index), next(nullptr) {}
    };

    Node* memoryList; 

public:
    HeapManager() : memoryList(nullptr) {
        pthread_mutex_init(&mtx, NULL);
    
        pthread_mutex_init(&printMutex, NULL);
    }
    ~HeapManager() {
        pthread_mutex_destroy(&mtx); 
    
        pthread_mutex_destroy(&printMutex);
    }

    void initHeap(int size);

    int myMalloc(int ID, int size);

    int myFree(int ID, int index);

    void display();

private:

    Node* createNode(int id, int size, int index);

    void coalesceFreeBlocks();
};

void HeapManager::initHeap(int size) {

    memoryList = createNode(-1, size, 0);
    pthread_mutex_lock(&printMutex);
    cout<<"Memory initialized"<<endl;
    display();
    pthread_mutex_unlock(&printMutex);
}

int HeapManager::myMalloc(int ID, int size) {
    pthread_mutex_lock(&mtx);

    Node* current = memoryList;
    while (current != nullptr) {
        if (current->ID == -1 && current->SIZE >= size) {
            if (current->SIZE > size) {
                Node* newFreeBlock = createNode(-1, current->SIZE - size, current->INDEX + size);
                newFreeBlock->next = current->next;
                current->SIZE = size;
                current->next = newFreeBlock;
            }

            current->ID = ID;
            pthread_mutex_lock(&printMutex);
            cout << "Allocated for thread " << ID << endl;
            display();
            pthread_mutex_unlock(&printMutex);
            pthread_mutex_unlock(&mtx);
            return current->INDEX;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&mtx);
    return -1; 
}

int HeapManager::myFree(int ID, int index) {
    pthread_mutex_lock(&mtx);

    Node* current = memoryList;
    while (current != nullptr) {
        if (current->ID == ID && current->INDEX == index) {
            current->ID = -1;
            coalesceFreeBlocks();
            pthread_mutex_lock(&printMutex);
            std::cout << "Freed for thread " << ID << "\n";
            display();
            pthread_mutex_unlock(&printMutex);
            pthread_mutex_unlock(&mtx); 
            return 1;
        }
        current = current->next;
    }
    pthread_mutex_lock(&printMutex);
    std::cout << "Free failed for thread " << ID << ".\n";
    display();
    pthread_mutex_unlock(&printMutex);
    pthread_mutex_unlock(&mtx); 
    return -1;
}

void HeapManager::display() {
    Node* current = memoryList;
    while (current != nullptr) {
        std::cout <<"ID: [" << current->ID << "]"<<" Size: [" << current->SIZE << "] Index: [" << current->INDEX << "]";
        if (current->next != nullptr) {
            std::cout << "---";
        }
        current = current->next;
    }
    std::cout << "\n";
}

HeapManager::Node* HeapManager::createNode(int id, int size, int index) {
    return new Node(id, size, index);
}

void HeapManager::coalesceFreeBlocks() {
    Node* current = memoryList;
    Node* lastFreeBlock = nullptr;

    while (current != nullptr) {
        if (current->ID == -1) {
            if (lastFreeBlock != nullptr && lastFreeBlock->INDEX + lastFreeBlock->SIZE == current->INDEX) {
               
                lastFreeBlock->SIZE += current->SIZE;
                lastFreeBlock->next = current->next;
                delete current;
                current = lastFreeBlock->next;
                continue;  
            } else {
                lastFreeBlock = current;
            }
        } else {
            lastFreeBlock = nullptr;
        }

        current = current->next;
    }
}



