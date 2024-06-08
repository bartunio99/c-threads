#include <ncurses.h>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>


using namespace std;

mutex mtx;  //mutex
condition_variable cv;  

//Global variables
vector<vector<string>> screen(7, vector<string>(1, "     |   |_____[S]__"));
bool running = true;   //is program running
bool isFree = false;    //can customer enter elevator
int currentFloor = 0;   //from 0 to 6
int previousFloor;

//constant helper variables
int const elevatorColumn = 7;   //elevator column
int const startingFloor = 0;

//prints all screen contents
void printScreen()
{
    erase(); 

    for (int i = 0; i < screen.size(); ++i)
    {
        for (int j = 0; j < screen[i].size(); ++j)
        {
            string line = screen[i][j];
            for (int k = 0; k < line.size(); ++k)
            {
                mvaddch(i, k, line[k]);
            }
        }
    }
    refresh();
}

//updates screen every 25ms
void updateScreen()
{
    while (running)
    {
        printScreen();
        this_thread::sleep_for(chrono::milliseconds(25));
    }
}

//elevator thread
void elevator(){
    while(running){
        previousFloor = currentFloor;
        currentFloor = (1 + currentFloor)%screen.size() ;

        
        if(currentFloor == 0){
            mtx.lock();
            isFree = true;
            mtx.unlock();
        }else if(currentFloor == 1){
            mtx.lock();
            isFree = false;
            mtx.unlock();           
        }

        mtx.lock();
        cv.notify_all();
        screen[previousFloor][0][elevatorColumn - 1] = ' ';
        screen[previousFloor][0][elevatorColumn ] = ' ';
        screen[previousFloor][0][elevatorColumn + 1] = ' ';

        screen[currentFloor][0][elevatorColumn - 1] = '_';
        screen[currentFloor][0][elevatorColumn ] = '_';
        screen[currentFloor][0][elevatorColumn + 1] = '_';
        mtx.unlock();

    
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

//customer thread
void customer(char symbol){
    int speed = rand() % 10 + 1;    //od 1 do 10
    int floor = rand() % 6 + 1;     //random floor from 1 to 6
    int waitTime = 5000;
    int customerX = 0;


    //walk to elevator 
    {
        unique_lock<mutex> lock(mtx);
        screen[startingFloor][0][customerX] = symbol;
    }

    while(customerX < 4){   
        {
            unique_lock<mutex> lock(mtx);
            screen[startingFloor][0][customerX+1] = symbol;
            screen[startingFloor][0][customerX] = ' ';
        }
        customerX++;
        this_thread::sleep_for(chrono::milliseconds(10000/speed));
    }

    //wait for elevator
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock,[]{return isFree;});
        screen[startingFloor][0][customerX] = ' ';
    }

    //wait for elevator to reach floor
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock,[&]{return currentFloor==floor;});
        customerX = 10;                             //exit elevator
        screen[floor][0][customerX] = symbol;
    }

    while(customerX < 14){   //walk to stand
        {
            unique_lock<mutex> lock(mtx);
            screen[floor][0][customerX+1] = symbol;
            screen[floor][0][customerX] = '_';
        }
        customerX++;
        this_thread::sleep_for(chrono::milliseconds(10000/speed));
    }

    this_thread::sleep_for(chrono::milliseconds(waitTime)); //appointment time

    {
        unique_lock<mutex> lock(mtx);
        screen[floor][0][customerX] = '_';  //exit system
    }
}

//thread that creates customer
void makeCustomers(){
    //ASCII values from 97 (a) to 122 (z)

    int ASCIImin = 97;
    int ASCIImax = 122;

    int currentIndex = ASCIImin;

    while(running){
        if(currentIndex > ASCIImax){
            currentIndex = ASCIImin;
        }

        thread customerThread(customer, char(currentIndex));
        customerThread.join();
        currentIndex++;
        this_thread::sleep_for(chrono::milliseconds(rand()%10000 + 5000));
    } 
}


int main(){
    //screen init
    initscr();
    noecho();
    curs_set(0);
    printScreen();

    //random number seed
    srand(time(NULL));

    //threads
    thread screenThread(updateScreen);
    thread elevatorThread(elevator);
    thread makeCustomersThread(makeCustomers);
    //main loop, ends all threads when space is clicked
    while(true){
        char c = getch();
        if(c == ' '){  
            //end of threads
            running = false;
            if(screenThread.joinable()){
                screenThread.join();
            }
            if(elevatorThread.joinable()){
                elevatorThread.join();
            }
            if(makeCustomersThread.joinable()){
                makeCustomersThread.join();
            }

            endwin();
            return 0;
        }
    }
    
}
