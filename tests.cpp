#include "elevator-fsm.cpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

// Mock the API's the FSM uses.
// Tests do not know the specific order of API calls, so EXPECT_CALL macros to
// them are ordered by class, matching the order of these declarations.
class MockElevatorUi : public ElevatorUiApi
{
public:
    MockElevatorUi(){}

    MOCK_METHOD(void, arrived, (size_t floor), (override));
    MOCK_METHOD(void, inService, (), (override));
    MOCK_METHOD(void, outOfService, (), (override));
    MOCK_METHOD(void, alarmOn, (), (override));
    MOCK_METHOD(void, alarmOff, (), (override));

    bool mockFloorRequest(size_t floor) { return client_->handleFloorRequest(floor); }
    bool mockOpenButtonEvent()          { return client_->handleOpenButton(); }
    bool mockCloseButtonEvent()         { return client_->handleCloseButton(); }
    bool mockStopButtonEvent()          { return client_->handleStopButton(); }
    bool mockRestoreServiceEvent()      { return client_->handleRestoreService(); }
};

class MockElevatorDoor : public ElevatorDoorApi
{
public:
    MockElevatorDoor() {}

    MOCK_METHOD(void, open, (), (override));
    MOCK_METHOD(void, close, (), (override));

    bool mockOpenedEvent() { return client_->handleOpened(); }
    bool mockClosedEvent() { return client_->handleClosed(); }
    bool mockFaultEvent()  { return client_->handleDoorFault(); }
};

class MockElevatorDrive : public ElevatorDriveApi
{
public:
    MockElevatorDrive() {}

    MOCK_METHOD(void,   goToFloor,(size_t floor), (override));
    MOCK_METHOD(void,   stop, (), (override));
    MOCK_METHOD(void,   start, (), (override));

    MOCK_METHOD(size_t, getFloor, (), (const, override));
    MOCK_METHOD(bool,   isAtFloor, (), (const, override));

    bool mockArrivedEvent() { return client_->handleArrived(); }
    bool mockFaultEvent()   { return client_->handleDriveFault(); }
};

class MockElevatorTimer : public ElevatorTimerApi
{
public:
    MockElevatorTimer() {}

    MOCK_METHOD(void, start, (size_t msec), (override));
    MOCK_METHOD(void, stop, (), (override));

    bool mockExpired() { return client_->handleExpired(); }
};

//---------- Base test fixture used by all other fixtures ---------------------

class TestElevatorFsmBuilder: public ::testing::Test {
public:
    TestElevatorFsmBuilder()
    {
        EXPECT_CALL(ui_, inService());
        fsm_ = new ElevatorFsm(ui_, door_, drive_, timer_);
    }

    virtual ~TestElevatorFsmBuilder()
    {
        delete fsm_;
    }

    MockElevatorUi    ui_;
    MockElevatorDoor  door_;
    MockElevatorDrive drive_;
    MockElevatorTimer timer_;

    ElevatorFsm *fsm_;
};

//---------- Given_StoppedElevator --------------------------------------------

class Given_StoppedElevator: public TestElevatorFsmBuilder {
public:
    void SetUp( ) {
    }

    void TearDown( ) {
    }
};

TEST_F(Given_StoppedElevator, Should_BeIdle_When_NoActivity)
{
    ASSERT_TRUE(fsm_->isIdle());
}

TEST_F(Given_StoppedElevator, Should_NotBeIdle_When_SameFloorRequested)
{
    // Starts out idle.
    ASSERT_TRUE(fsm_->isIdle());

    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(door_, open());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));

    ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR));

    ASSERT_FALSE(fsm_->isIdle());
}

TEST_F(Given_StoppedElevator, Should_OpenDoor_When_SameFloorRequested)
{
    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(door_, open());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));

    ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR));
}

TEST_F(Given_StoppedElevator, Should_OpenDoor_When_OpenButtonPushed)
{
    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(door_, open());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));

    ASSERT_TRUE(ui_.mockOpenButtonEvent());
}

TEST_F(Given_StoppedElevator, Should_MoveToFloor_When_NewFloorRequested)
{
    EXPECT_CALL(drive_, goToFloor(ElevatorFsm::GROUND_FLOOR + 1));
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_MOVE_TO_FLOOR_MSEC));

    ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR + 1));
}

TEST_F(Given_StoppedElevator, Should_GoOutOfService_When_SameFloorRequestedAndDoorTimesOut)
{
    // Starts out in service.
    ASSERT_TRUE(fsm_->isInService());

    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(ui_, outOfService());
    EXPECT_CALL(door_, open());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));

    // Request same floor, then timeout door opening.
    ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR));
    ASSERT_TRUE(timer_.mockExpired());

    ASSERT_FALSE(fsm_->isInService());
}

TEST_F(Given_StoppedElevator, Should_GoOutOfService_When_SameFloorRequestedAndDoorFaults)
{
    // Starts out in service.
    ASSERT_TRUE(fsm_->isInService());

    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(ui_, outOfService());
    EXPECT_CALL(door_, open());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));

    // Request same floor, then fault door opening.
    ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR));
    ASSERT_TRUE(door_.mockFaultEvent());

    ASSERT_FALSE(fsm_->isInService());
}

//---------- Given_MovingElevator ----------------------------------------------

class Given_MovingElevator: public TestElevatorFsmBuilder {
public:
    void SetUp( ) {
        // Drive FSM into Moving state:
        // Request new floor.
        EXPECT_CALL(drive_, goToFloor(ElevatorFsm::GROUND_FLOOR + 1));
        EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_MOVE_TO_FLOOR_MSEC));

        ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR + 1));
    }

    void TearDown( ) {
    }
};

TEST_F(Given_MovingElevator, Should_BeWaiting_When_Arrived)
{
    // Starts out not waiting at floor.
    ASSERT_FALSE(fsm_->isWaiting());

    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR + 1));
    EXPECT_CALL(door_, open());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMER_WAITING_MSEC));

    // Signal arrival, then complete opening door.
    ASSERT_TRUE(drive_.mockArrivedEvent());
    ASSERT_TRUE(door_.mockOpenedEvent());

    ASSERT_TRUE(fsm_->isWaiting());
}

TEST_F(Given_MovingElevator, Should_Stop_When_StopButtonPushed)
{
    EXPECT_CALL(ui_, alarmOn());
    EXPECT_CALL(drive_, stop());

    ASSERT_TRUE(ui_.mockStopButtonEvent());
}

TEST_F(Given_MovingElevator, Should_Resume_When_StopButtonPushedTwice)
{
    EXPECT_CALL(ui_, alarmOn());
    EXPECT_CALL(ui_, alarmOff());
    EXPECT_CALL(drive_, stop());
    EXPECT_CALL(drive_, start());

    ASSERT_TRUE(ui_.mockStopButtonEvent());
    ASSERT_TRUE(ui_.mockStopButtonEvent());
}

TEST_F(Given_MovingElevator, Should_GoOutOfService_When_DriveTimesOut)
{
    // Starts out in service.
    ASSERT_TRUE(fsm_->isInService());

    EXPECT_CALL(ui_, outOfService());

    ASSERT_TRUE(timer_.mockExpired());

    ASSERT_FALSE(fsm_->isInService());
}

TEST_F(Given_MovingElevator, Should_GoOutOfService_When_DriveFaults)
{
    // Starts out in service.
    ASSERT_TRUE(fsm_->isInService());

    EXPECT_CALL(ui_, outOfService());

    ASSERT_TRUE(drive_.mockFaultEvent());

    ASSERT_FALSE(fsm_->isInService());
}

//---------- Given_WaitingElevator ----------------------------------------------

class Given_WaitingElevator: public TestElevatorFsmBuilder {
public:
    void SetUp( ) {
        // Drive FSM into Waiting state:
        // Request same floor, then complete opening door.
        EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
        EXPECT_CALL(door_, open());
        EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));
        EXPECT_CALL(timer_, start(ElevatorFsm::TIMER_WAITING_MSEC));

        ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR));
        ASSERT_TRUE(door_.mockOpenedEvent());

        ASSERT_FALSE(fsm_->isIdle());
    }

    void TearDown( ) {
    }
};

TEST_F(Given_WaitingElevator, Should_KeepDoorOpen_When_OpenButtonPushed)
{
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMER_WAITING_MSEC));

    ASSERT_TRUE(ui_.mockOpenButtonEvent());
}

TEST_F(Given_WaitingElevator, Should_CloseDoor_When_TimerExpires)
{
    EXPECT_CALL(door_, close());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_CLOSE_MSEC));

    ASSERT_TRUE(timer_.mockExpired());
}

TEST_F(Given_WaitingElevator, Should_CloseDoor_When_CloseButtonPushed)
{
    EXPECT_CALL(door_, close());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_CLOSE_MSEC));

    ASSERT_TRUE(ui_.mockCloseButtonEvent());
}

TEST_F(Given_WaitingElevator, Should_BeIdle_When_DoorCloses)
{
    EXPECT_CALL(door_, close());
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_CLOSE_MSEC));

    // Complete waiting time, then complete closing door.
    ASSERT_TRUE(timer_.mockExpired());
    ASSERT_TRUE(door_.mockClosedEvent());

    ASSERT_TRUE(fsm_->isIdle());
}

//---------- Given_WaitingElevator ----------------------------------------------

class Given_OutOfServiceElevator: public TestElevatorFsmBuilder {
public:
    void SetUp( ) {
        // Drive FSM into OutOfService state:
        // Request new floor, then fault drive.
        EXPECT_CALL(ui_, outOfService());
        EXPECT_CALL(drive_, goToFloor(ElevatorFsm::GROUND_FLOOR + 1));
        EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_MOVE_TO_FLOOR_MSEC));

        ASSERT_TRUE(ui_.mockFloorRequest(ElevatorFsm::GROUND_FLOOR + 1));
        ASSERT_TRUE(drive_.mockFaultEvent());

        ASSERT_FALSE(fsm_->isInService());
        ASSERT_FALSE(fsm_->isIdle());
    }

    void TearDown( ) {
    }
};

TEST_F(Given_OutOfServiceElevator, Should_BeWaiting_When_ServiceRestoredAtGroundFloor)
{
    // Return to service at ground floor, then complete opening door.
    EXPECT_CALL(ui_, arrived(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(ui_, inService());
    EXPECT_CALL(door_, open());
    EXPECT_CALL(drive_, getFloor())
        .WillOnce(Return(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(drive_, isAtFloor())
        .WillOnce(Return(true));
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_DOOR_OPEN_MSEC));
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMER_WAITING_MSEC));

    ASSERT_TRUE(ui_.mockRestoreServiceEvent());
    ASSERT_TRUE(door_.mockOpenedEvent());

    ASSERT_TRUE(fsm_->isInService());
    ASSERT_TRUE(fsm_->isWaiting());
}

TEST_F(Given_OutOfServiceElevator, Should_MoveToGround_When_ServiceRestoredOffGroundFloor)
{
    // Return to service off ground floor (i.e. in between ground and next
    // floor).
    EXPECT_CALL(ui_, inService());
    EXPECT_CALL(drive_, getFloor())
        .WillOnce(Return(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(drive_, isAtFloor())
        .WillOnce(Return(false));
    EXPECT_CALL(drive_, goToFloor(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_MOVE_TO_FLOOR_MSEC));

    ASSERT_TRUE(ui_.mockRestoreServiceEvent());

    ASSERT_TRUE(fsm_->isInService());
}

TEST_F(Given_OutOfServiceElevator, Should_MoveToGround_When_ServiceRestoredAtOtherFloor)
{
    // Return to service at non-ground floor.
    EXPECT_CALL(ui_, inService());
    EXPECT_CALL(drive_, getFloor())
        .WillOnce(Return(ElevatorFsm::GROUND_FLOOR + 1));
    EXPECT_CALL(drive_, isAtFloor())
        .WillOnce(Return(false));
    EXPECT_CALL(drive_, goToFloor(ElevatorFsm::GROUND_FLOOR));
    EXPECT_CALL(timer_, start(ElevatorFsm::TIMEOUT_MOVE_TO_FLOOR_MSEC));

    ASSERT_TRUE(ui_.mockRestoreServiceEvent());

    ASSERT_TRUE(fsm_->isInService());
}

//---------- Main program -----------------------------------------------------

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

