#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>
#include <queue>
#include <limits>
#include <algorithm>

struct Restaurant 
{
    std::string name;
    double distance;
};

std::mutex mtx;
std::condition_variable cv;
std::queue<Restaurant> inputQueue;
std::vector<Restaurant> filteredRestaurants;
bool done = false;
double maxDistance;

void restaurantWorker() 
{
    std::random_device rd;
    std::mt19937 gen(rd());

    while (true) 
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock);

        if (done && inputQueue.empty()) 
            break;
        if (inputQueue.empty()) 
            continue;

        while (!inputQueue.empty()) 
        {
            Restaurant r = inputQueue.front();
            inputQueue.pop();

            if (r.distance <= maxDistance) 
            {
                filteredRestaurants.push_back(r);

                if (!filteredRestaurants.empty()) 
                {
                    std::uniform_int_distribution<> dist(0, filteredRestaurants.size() - 1);
                    std::cout << " \nSuggested place: " << filteredRestaurants[dist(gen)].name << "\n\n";
                }
                std::cout << "Added \"" << r.name << "\" (" << r.distance << " miles) to nearby list.\n\n";
            } 
            else 
            {
                std::cout << r.name << " is too far (" << r.distance << " miles).\n\n";
            }
        }
    }
}

int main() 
{
    std::cout << "Restaurant Finder \n\n";
    std::cout << "Available commands:\n";
    std::cout << "  - Type a restaurant name to add it.\n";
    std::cout << "  - Type 'list' to see all nearby restaurants saved so far.\n";
    std::cout << "  - Type 'exit' to quit the program.\n\n";

    std::cout << "Enter maximum search radius in miles: ";
    std::cin >> maxDistance;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

    std::thread worker(restaurantWorker);

    while (true) 
    {
        Restaurant r;
        std::cout << "\nEnter restaurant name (or 'list'/'exit'): ";
        std::getline(std::cin, r.name);

        r.name.erase(0, r.name.find_first_not_of(" \t\n\r\f\v"));
        r.name.erase(r.name.find_last_not_of(" \t\n\r\f\v") + 1);

        if (r.name.empty()) 
        {
            std::cout << "Restaurant name cannot be blank. Please try again.\n";
            continue;
        }

        if (r.name == "exit") 
        {
            std::string confirm;
            std::cout << "Are you sure you want to exit? (yes/no): ";
            std::getline(std::cin, confirm);

            std::transform(confirm.begin(), confirm.end(), confirm.begin(), [](unsigned char c){ return std::tolower(c); });

            if (confirm == "yes" || confirm == "y") 
                break;
            else 
            {
                std::cout << "Returning to input...\n";
                continue;
            }
        }

        if (r.name == "list") 
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (filteredRestaurants.empty()) 
            {
                std::cout << "No nearby restaurants saved yet.\n";
            } 
            else 
            {
                std::cout << "Nearby Restaurants:\n";
                for (size_t i = 0; i < filteredRestaurants.size(); i++) 
                {
                    std::cout << i + 1 << ". " << filteredRestaurants[i].name
                              << " (" << filteredRestaurants[i].distance << " miles)\n";
                }
            }
            continue;
        }

        std::cout << "Enter distance in miles: ";
        std::cin >> r.distance;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

        {
            std::unique_lock<std::mutex> lock(mtx);
            inputQueue.push(r);
        }
        cv.notify_one();
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_one();
    worker.join();

    std::cout << "\nProgram Closed - Total nearby restaurants saved: " << filteredRestaurants.size() << "\n";
    for (size_t i = 0; i < filteredRestaurants.size(); i++) 
    {
        std::cout << i + 1 << ". " << filteredRestaurants[i].name 
                  << " (" << filteredRestaurants[i].distance << " miles)\n";
    }
    return 0;
}
