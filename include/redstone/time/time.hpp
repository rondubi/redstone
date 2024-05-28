#pragma once

#include <chrono>
#include <queue>
#include <functional>
#include <mutex>

namespace redstone::time {
    class time_manager {
    public:
        // Scale: simulation time / real time
        explicit time_manager(double scale);

        // Sleep for a duration scaled to simulation time
        void sleep(std::chrono::milliseconds sim_ms);

        // Get the current time in simulation time
        std::chrono::time_point<std::chrono::system_clock> clock_gettime();

        // Set a timeout to call a function after a duration in simulation time
        void set_timeout(std::chrono::milliseconds sim_ms, std::function<void()> callback);

        // Process all timeouts that have expired
        void process_timeouts();

    private:
        double scale;
        std::chrono::time_point<std::chrono::system_clock> initial_time;

        struct timeout {
            std::chrono::time_point<std::chrono::system_clock> time;
            std::function<void()> callback;

            bool operator<(const timeout& other) const {
                return time > other.time; // Reverse order
            }
        };

        std::priority_queue<timeout> timeouts;
        std::mutex timeout_mutex;
    };
}