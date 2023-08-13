#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <chrono>
#include <future>

using namespace std::chrono;

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this]
               { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    std::cout << "   Message " << msg << " will be added to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
    _currentPhase = TrafficLight::TrafficLightPhase::RED;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (_msgQueue->receive() == TrafficLight::TrafficLightPhase::GREEN)
        {
            break;
        }
    }
    
    return;
}

TrafficLight::TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    high_resolution_clock::time_point currentTime, prevTime;
    // Generate random number
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4, 6);
    int64_t randomTime = distr(eng) * 1000;
    int64_t timeSpan;

    prevTime = high_resolution_clock::now();

    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        currentTime = high_resolution_clock::now();
        timeSpan = duration_cast<milliseconds>(currentTime - prevTime).count();
        if (timeSpan >= randomTime)
        {
            // toggle light
            _currentPhase = (TrafficLight::TrafficLightPhase)((_currentPhase + 1) & 0x0001);

            // push traffic light's status to queue
            TrafficLight::TrafficLightPhase msg = _currentPhase;
            std::future<void> future = std::async(std::launch::async, &MessageQueue<TrafficLight::TrafficLightPhase>::send, _msgQueue, std::move(msg));
            future.wait();

            prevTime = high_resolution_clock::now();
            randomTime = distr(eng) * 1000;
        }
    }
}