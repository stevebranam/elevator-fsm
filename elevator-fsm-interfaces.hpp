// Abstract interfaces used by the Elevator FSM.
//
// Author: Steve Branam, sdbranam@gmail.com, December, 2021
//
#ifndef ELEVATOR_FSM_INTERFACES_HPP
#define ELEVATOR_FSM_INTERFACES_HPP

#include <cstddef>

//---------- Client interfaces the FSM implements. ----------------------------

//---------- Elevator User Interface Events -----------------------------------
class ElevatorUiClient
{
public:
    ElevatorUiClient() {}

    virtual bool handleFloorRequest(size_t floor) = 0;
    virtual bool handleOpenButton() = 0;
    virtual bool handleCloseButton() = 0;
    virtual bool handleStopButton() = 0;
    virtual bool handleRestoreService() = 0;
};

//---------- Elevator Door Controller Events ----------------------------------
class ElevatorDoorClient
{
public:
    ElevatorDoorClient() {}

    virtual bool handleOpened() = 0;
    virtual bool handleClosed() = 0;
    virtual bool handleDoorFault() = 0;
};

//---------- Elevator Drive Controller Events ---------------------------------
class ElevatorDriveClient
{
public:
    ElevatorDriveClient() {}

    virtual bool handleArrived() = 0;
    virtual bool handleDriveFault() = 0;
};

//---------- Elevator Timer Events --------------------------------------------
class ElevatorTimerClient
{
public:
    ElevatorTimerClient() {}

    virtual bool handleExpired() = 0;
};

//---------- API's the FSM uses. ----------------------------------------------

//---------- Elevator User Interface ------------------------------------------
class ElevatorUiApi
{
public:
    ElevatorUiApi()
        : client_(nullptr)
        {}

    void init(ElevatorUiClient *client)
    {
        client_ = client;
    }
    bool isInitDone() const { return client_ != nullptr; }

    virtual void arrived(size_t floor) = 0;
    virtual void inService() = 0;
    virtual void outOfService() = 0;
    virtual void alarmOn() = 0;
    virtual void alarmOff() = 0;

protected:
    ElevatorUiClient *client_;
};

//---------- Elevator Door Controller -----------------------------------------
class ElevatorDoorApi
{
public:
    ElevatorDoorApi()
        : client_(nullptr)
        {}

    void init(ElevatorDoorClient *client)
    {
        client_ = client;
    }
    bool isInitDone() const { return client_ != nullptr; }

    virtual void open() = 0;
    virtual void close() = 0;

protected:
    ElevatorDoorClient *client_;
};

//---------- Elevator Drive Controller ----------------------------------------
class ElevatorDriveApi
{
public:
    ElevatorDriveApi()
        : client_(nullptr)
        {}

    void init(ElevatorDriveClient *client)
    {
        client_ = client;
    }
    bool isInitDone() const { return client_ != nullptr; }

    virtual void   goToFloor(size_t floor) = 0;
    virtual void   stop() = 0;
    virtual void   start() = 0;
    virtual size_t getFloor() const = 0;  // Floor at or below car.
    virtual bool   isAtFloor() const = 0; // Car is actually at floor.

protected:
    ElevatorDriveClient *client_;
};

//---------- Elevator Timer ---------------------------------------------------
class ElevatorTimerApi
{
public:
    ElevatorTimerApi()
        : client_(nullptr)
        {}

    void init(ElevatorTimerClient *client)
    {
        client_ = client;
    }
    bool isInitDone() const { return client_ != nullptr; }

    // Starting the times when it is already running restarts it.
    // Stopping the timer when it is already stopped is a no-op.
    virtual void start(size_t msec) = 0;
    virtual void stop() = 0;

protected:
    ElevatorTimerClient *client_;
};
#endif // ELEVATOR_FSM_INTERFACES_HPP
