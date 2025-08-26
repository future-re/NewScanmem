#include <unistd.h>

#include <gtest/gtest.h>

import process_checker;

class ProcessCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ProcessCheckerTest, CheckCurrentProcess) {
    pid_t current_pid = getpid();

    ProcessState state = ProcessChecker::checkProcess(current_pid);

    EXPECT_EQ(state, ProcessState::RUNNING);
}

TEST_F(ProcessCheckerTest, CheckDeadProcess) {
    pid_t invalid_pid = 99999;
    
    ProcessState state = ProcessChecker::checkProcess(invalid_pid);
    
    EXPECT_EQ(state, ProcessState::DEAD);
}

TEST_F(ProcessCheckerTest, CheckInvalidPid) {
    pid_t invalid_pid = -1;
    
    ProcessState state = ProcessChecker::checkProcess(invalid_pid);
    
    EXPECT_EQ(state, ProcessState::ERROR);
}

TEST_F(ProcessCheckerTest, CheckZeroPid) {
    pid_t zero_pid = 0;
    
    ProcessState state = ProcessChecker::checkProcess(zero_pid);
    
    EXPECT_EQ(state, ProcessState::ERROR);
}

TEST_F(ProcessCheckerTest, IsProcessDeadCurrent) {
    pid_t current_pid = getpid();

    bool is_dead = ProcessChecker::isProcessDead(current_pid);

    EXPECT_FALSE(is_dead);
}

TEST_F(ProcessCheckerTest, IsProcessDeadInvalid) {
    pid_t invalid_pid = 99999;
    
    bool is_dead = ProcessChecker::isProcessDead(invalid_pid);
    
    EXPECT_TRUE(is_dead);
}

TEST_F(ProcessCheckerTest, ParseProcessState) {
    // Test through public interface since parse_process_state is private
    pid_t current_pid = getpid();

    ProcessState state = ProcessChecker::checkProcess(current_pid);

    // We expect the current process to be running
    EXPECT_EQ(state, ProcessState::RUNNING);
}