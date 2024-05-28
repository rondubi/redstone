#include "redstone/time/time.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace redstone::time {
    time_manager::time_manager(double scale) : scale(scale), initial_time(std::chrono::system_clock::now()) {}

    void time_manager::sleep(std::chrono::milliseconds sim_ms) {
        auto real_ms = std::chrono::duration_cast<std::chrono::milliseconds>(sim_ms / scale);
        std::this_thread::sleep_for(real_ms);
    }

    std::chrono::time_point<std::chrono::system_clock> time_manager::clock_gettime() {
        auto real_now = std::chrono::system_clock::now();
        auto elapsed_real_time = real_now - initial_time;
        auto sim_duration = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_real_time * scale);
        return std::chrono::time_point<std::chrono::system_clock>(sim_duration);
    }

    void time_manager::set_timeout(std::chrono::milliseconds sim_ms, std::function<void()> callback) {
        std::unique_lock<std::mutex> lock(timeout_mutex);
        timeouts.push({clock_gettime() + sim_ms, callback});
    }

    std::string time_point_to_string(const std::chrono::time_point<std::chrono::system_clock>& tp) {
        // Convert time_point to time_t
        std::time_t time = std::chrono::system_clock::to_time_t(tp);

        // Convert time_t to tm as UTC time
        std::tm tm = *std::gmtime(&time);

        // Create a string stream to format the time
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        return ss.str();
    }

    void time_manager::process_timeouts() {
        std::unique_lock<std::mutex> lock(timeout_mutex);
        auto current_time = clock_gettime();
        while (!timeouts.empty()) {
            if (timeouts.top().time > current_time) {
                break;
            }
            auto timeout = timeouts.top();
            timeouts.pop();
            timeout.callback();
        }
    }

} // namespace redstone::time