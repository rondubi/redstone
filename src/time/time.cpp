#include "redstone/time/time.hpp"

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

    void time_manager::process_timeouts() {
        std::unique_lock<std::mutex> lock(timeout_mutex);
        auto current_time = clock_gettime();
        while (!timeouts.empty() && timeouts.top().time <= current_time) {
            auto timeout = timeouts.top();
            timeouts.pop();
            timeout.callback();
        }
    }

} // namespace redstone::time