#include "redstone/time/time.hpp"
#include <chrono>
#include <gtest/gtest.h>

struct TimeManagerTest : testing::Test, testing::WithParamInterface<double> {
    double scale;
    TimeManagerTest() : scale(GetParam()) {}
};

TEST_P(TimeManagerTest, SleepTest) {
    redstone::time::time_manager manager(scale); // Set scale to 1.0
    std::chrono::milliseconds sim_ms(1000); // Simulate 1000 milliseconds

    auto start = std::chrono::system_clock::now();
    auto sim_start = manager.clock_gettime();
    manager.sleep(sim_ms);
    auto end = std::chrono::system_clock::now();
    auto sim_end = manager.clock_gettime();

    auto elapsed_real_time = end - start;
    auto elapsed_sim_time = sim_end - sim_start;
    auto elapsed_sim_time_scaled = elapsed_sim_time / scale;
    
    // Check that the elapsed real time is approximately equal to the elapsed
    EXPECT_NEAR(elapsed_real_time.count(), elapsed_sim_time_scaled.count(), 1000000);
}

INSTANTIATE_TEST_CASE_P(ScaleValues, TimeManagerTest, ::testing::Values(1.0, 0.5, 2.0, 10.0, 100.0, 1000.0));

TEST()

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}