#include "redstone/time/time.hpp"

namespace redstone::time {
    time::time(double scale) : scale(scale), initial_time(std::chrono::system_clock::now()) {}

    void time::sleep(std::chrono::milliseconds sim_ms) {
        std::this_thread::sleep_for(convert_to_real(ms));
    }

    std::chrono::time_point<std::chrono::system_clock> time::clock_gettime() {
        return initial_time + convert_to_sim(std::chrono::system_clock::now() - initial_time);
    }

    void time::set_timeout(std::chrono::milliseconds sim_ms, std::function<void()> callback) {
        std::unique_lock<std::mutex> lock(timeout_mutex);
        timeouts.push({clock_gettime() + sim_ms, callback});
    }

    void time::process_timeouts() {
        std::unique_lock<std::mutex> lock(timeout_mutex);
        auto current_time = clock_gettime();
        while (!timeouts.empty() && timeouts.top().time <= current_time) {
            auto timeout = timeouts.top();
            timeouts.pop();
            timeout.callback();
        }
    }

    std::chrono::duration<double, std::milli> time::convert_to_real(std::chrono::milliseconds sim_ms) {
        return std::chrono::duration<double, std::milli>(ms.count() * scale);
    }

    std::chrono::duration<double, std::milli> time::convert_to_sim(std::chrono::milliseconds real_ms) {
        return std::chrono::duration<double, std::milli>(ms.count() / scale);
    }

} // namespace redstone::time