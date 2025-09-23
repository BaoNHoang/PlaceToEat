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

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !inputQueue.empty() || done; });

        if (done && inputQueue.empty()) break;

        while (!inputQueue.empty())
        {
            Restaurant r = inputQueue.front();
            inputQueue.pop();

            lock.unlock(); 

            if (r.distance <= maxDistance) 
            {
                {
                    std::unique_lock<std::mutex> innerLock(mtx);
                    filteredRestaurants.push_back(r);
                }
                std::cout << "Added \"" << r.name 
                          << "\" (" << r.distance << " miles) to nearby list.\n";

                std::unique_lock<std::mutex> suggestionLock(mtx);
                std::uniform_int_distribution<> dist(0, filteredRestaurants.size() - 1);
                std::cout << "Suggested place: " 
                          << filteredRestaurants[dist(gen)].name << "\n";
            } else 
            {
                std::cout << r.name << " is too far" 
                          << r.distance << " miles).\n";
            }

            lock.lock(); 
        }
    }
}

int main() 
{
    std::cout << "Restaurant Finder \n\n";
    std::cout << "Enter maximum search radius in miles: ";
    std::cin >> maxDistance;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::thread worker(restaurantWorker);

    while (true) 
    {
        Restaurant r;
        std::cout << "\nEnter restaurant name (or type 'exit' to quit): ";
        std::getline(std::cin, r.name);

        if (r.name == "exit") 
        {
            std::string confirm;
            std::cout << "Are you sure you want to exit? (yes/no): ";
            std::getline(std::cin, confirm);

            for (char &c : confirm) 
            {
                c = std::tolower(static_cast<unsigned char>(c));
            }

            if (confirm == "yes" || confirm == "y") 
            {
                break;
            } else 
            {
                std::cout << "Returning to input...\n";
                continue;
            }
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
