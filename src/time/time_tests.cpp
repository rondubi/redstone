#include "redstone/time/time.hpp"
#include <chrono>
#include <gtest/gtest.h>

struct TimeManagerTest : testing::Test, testing::WithParamInterface<double> {
    double scale;
    TimeManagerTest() : scale(GetParam()) {}
};

TEST_P(TimeManagerTest, SleepTest) {
    redstone::time::time_manager tm(scale); // Set scale to 1.0
    std::chrono::milliseconds sim_ms(1000); // Simulate 1000 milliseconds

    auto start = std::chrono::system_clock::now();
    auto sim_start = tm.clock_gettime();
    tm.sleep(sim_ms);
    auto end = std::chrono::system_clock::now();
    auto sim_end = tm.clock_gettime();

    auto elapsed_real_time = end - start;
    auto elapsed_sim_time = sim_end - sim_start;
    auto elapsed_sim_time_scaled = elapsed_sim_time / scale;
    
    // Check that the elapsed real time is approximately equal to the elapsed
    EXPECT_NEAR(elapsed_real_time.count(), elapsed_sim_time_scaled.count(), 1000000);
}

TEST_P(TimeManagerTest, GetTimeTest) {
    redstone::time::time_manager tm(scale);
    auto start = std::chrono::system_clock::now();
    auto sim_start = tm.clock_gettime();
    auto end = std::chrono::system_clock::now();
    auto sim_end = tm.clock_gettime();

    auto elapsed_real_time = end - start;
    auto elapsed_sim_time = sim_end - sim_start;
    auto elapsed_sim_time_scaled = elapsed_sim_time / scale;

    EXPECT_NEAR(elapsed_real_time.count(), elapsed_sim_time_scaled.count(), 1000000);
}

TEST_P(TimeManagerTest, SingleTimeout) {
    redstone::time::time_manager tm(scale);
    bool callback_called = false;
    std::chrono::milliseconds sim_ms(1000);
    tm.set_timeout(sim_ms, [&callback_called]() {
        callback_called = true;
    });

    std::chrono::milliseconds sleep_time(1250);
    std::this_thread::sleep_for(sleep_time / scale);
    tm.process_timeouts();

    EXPECT_TRUE(callback_called);
}

TEST_P(TimeManagerTest, MultipleTimeouts) {
    redstone::time::time_manager tm(scale);
    bool callback1_called = false;
    bool callback2_called = false;

    std::chrono::milliseconds sim_ms1(1000);
    tm.set_timeout(sim_ms1, [&callback1_called]() {
        callback1_called = true;
    });

    std::chrono::milliseconds sim_ms2(2000);
    tm.set_timeout(sim_ms2, [&callback2_called]() {
        callback2_called = true;
    });

    std::chrono::milliseconds sleep_time1(1250);
    std::this_thread::sleep_for(sleep_time1 / scale);
    tm.process_timeouts();
    EXPECT_TRUE(callback1_called);
    EXPECT_FALSE(callback2_called);

    std::chrono::milliseconds sleep_time2(1000);
    std::this_thread::sleep_for(sleep_time2 / scale);
    tm.process_timeouts();
    EXPECT_TRUE(callback2_called);
}

TEST_P(TimeManagerTest, NoTimeouts) {
    redstone::time::time_manager tm(scale);
    // Process timeouts when none are set
    tm.process_timeouts();
    // No expectations, just ensuring no crashes or unexpected behavior
}

TEST_P(TimeManagerTest, OutOfOrderTimeouts) {
    redstone::time::time_manager tm(scale);
    bool callback1_called = false;
    bool callback2_called = false;

    std::chrono::milliseconds sim_ms1(2000);
    tm.set_timeout(sim_ms1, [&callback2_called]() {
        callback2_called = true;
    });

    std::chrono::milliseconds sim_ms2(1000);
    tm.set_timeout(sim_ms2, [&callback1_called]() {
        callback1_called = true;
    });

    std::chrono::milliseconds sleep_time1(1250);
    std::this_thread::sleep_for(sleep_time1 / scale);
    tm.process_timeouts();
    EXPECT_TRUE(callback1_called);
    EXPECT_FALSE(callback2_called);

    std::chrono::milliseconds sleep_time2(1000);
    std::this_thread::sleep_for(sleep_time2 / scale);
    tm.process_timeouts();
    EXPECT_TRUE(callback2_called);
}

INSTANTIATE_TEST_CASE_P(ScaleValues, TimeManagerTest, ::testing::Values(1.0, 0.5, 0.1, 2.0, 10.0, 100.0, 1000.0));

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}