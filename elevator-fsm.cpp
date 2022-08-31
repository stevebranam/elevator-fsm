// Elevator FSM: An example FSM using the State pattern.
//
// Author: Steve Branam, sdbranam@gmail.com, December, 2021
//
#include "elevator-fsm.hpp"

//---------- Class ElevatorFsm Implementation ---------------------------------

ElevatorFsm::ElevatorFsm(
        ElevatorUiApi    &ui,
        ElevatorDoorApi  &door,
        ElevatorDriveApi &drive,
        ElevatorTimerApi &timer)
        : ui_(ui)
        , door_(door)
        , drive_(drive)
        , timer_(timer)
        , state_(ElevatorFsm::Stopped::instance())
        , currentFloor_(GROUND_FLOOR)
        , destinationFloor_(GROUND_FLOOR)
{
    ui_.init(this);
    door_.init(this);
    drive_.init(this);
    timer_.init(this);

    // All initialized, indicate system in service.
    ui_.inService();
}

bool ElevatorFsm::isInService() const
{
    return state_ != ElevatorFsm::OutOfService::instance();
}

bool ElevatorFsm::isIdle() const
{
    return state_ == ElevatorFsm::Stopped::instance();
}

bool ElevatorFsm::isWaiting() const
{
    return state_ == ElevatorFsm::Waiting::instance();
}

//---------- Class ElevatorFsm::State Implementation --------------------------

bool ElevatorFsm::State::changeState(ElevatorFsm *fsm, State *newState)
{
    fsm->state_ = newState;
    return fsm->state_->enter(fsm);
}

//---------- Class ElevatorFsm::Stopped Implementation ------------------------

bool ElevatorFsm::Stopped::enter(ElevatorFsm *fsm)
{
    return true;
}

bool ElevatorFsm::Stopped::onFloorRequest(ElevatorFsm *fsm)
{
    bool result = false;

    if (fsm->currentFloor_ == fsm->destinationFloor_)
    {
        result = changeState(fsm, Opening::instance());
    }
    else
    {
        result = changeState(fsm, Moving::instance());
    }

    return result;
}

bool ElevatorFsm::Stopped::onOpenButton(ElevatorFsm *fsm)
{
    return changeState(fsm, Opening::instance());
}

//---------- Class ElevatorFsm::Moving Implementation ------------------------

bool ElevatorFsm::Moving::enter(ElevatorFsm *fsm)
{
    fsm->drive_.goToFloor(fsm->destinationFloor_);
    fsm->timer_.start(TIMEOUT_MOVE_TO_FLOOR_MSEC);
    return true;
}

bool ElevatorFsm::Moving::onArrived(ElevatorFsm *fsm)
{
    return changeState(fsm, Opening::instance());
}

bool ElevatorFsm::Moving::onStopButton(ElevatorFsm *fsm)
{
    return changeState(fsm, Holding::instance());
}

bool ElevatorFsm::Moving::onFault(ElevatorFsm *fsm)
{
    return changeState(fsm, OutOfService::instance());
}

bool ElevatorFsm::Moving::onTimer(ElevatorFsm *fsm)
{
    return changeState(fsm, OutOfService::instance());
}

//---------- Class ElevatorFsm::Holding Implementation ------------------------

bool ElevatorFsm::Holding::enter(ElevatorFsm *fsm)
{
    fsm->drive_.stop();
    fsm->ui_.alarmOn();
    return true;
}

bool ElevatorFsm::Holding::onStopButton(ElevatorFsm *fsm)
{
    return changeState(fsm, Resuming::instance());
}

//---------- Class ElevatorFsm::Resuming Implementation ------------------------

bool ElevatorFsm::Resuming::enter(ElevatorFsm *fsm)
{
    fsm->drive_.start();
    fsm->ui_.alarmOff();
    return true;
}

//---------- Class ElevatorFsm::Opening Implementation ------------------------

bool ElevatorFsm::Opening::enter(ElevatorFsm *fsm)
{
    fsm->ui_.arrived(fsm->destinationFloor_);
    fsm->door_.open();
    fsm->timer_.start(TIMEOUT_DOOR_OPEN_MSEC);
    return true;
}

bool ElevatorFsm::Opening::onDoorsOpened(ElevatorFsm *fsm)
{
    return changeState(fsm, Waiting::instance());
}

bool ElevatorFsm::Opening::onFault(ElevatorFsm *fsm)
{
    return changeState(fsm, OutOfService::instance());
}

bool ElevatorFsm::Opening::onTimer(ElevatorFsm *fsm)
{
    return changeState(fsm, OutOfService::instance());
}

//---------- Class ElevatorFsm::Waiting Implementation ------------------------

bool ElevatorFsm::Waiting::enter(ElevatorFsm *fsm)
{
    fsm->timer_.start(TIMER_WAITING_MSEC);
    return true;
}

bool ElevatorFsm::Waiting::onOpenButton(ElevatorFsm *fsm)
{
    return changeState(fsm, Waiting::instance());
}

bool ElevatorFsm::Waiting::onCloseButton(ElevatorFsm *fsm)
{
    return changeState(fsm, Closing::instance());
}

bool ElevatorFsm::Waiting::onTimer(ElevatorFsm *fsm)
{
    return changeState(fsm, Closing::instance());
}

//---------- Class ElevatorFsm::Closing Implementation ------------------------

bool ElevatorFsm::Closing::enter(ElevatorFsm *fsm)
{
    fsm->door_.close();
    fsm->timer_.start(TIMEOUT_DOOR_CLOSE_MSEC);
    return true;
}

bool ElevatorFsm::Closing::onDoorsClosed(ElevatorFsm *fsm)
{
    return changeState(fsm, Stopped::instance());
}

bool ElevatorFsm::Closing::onFault(ElevatorFsm *fsm)
{
    return changeState(fsm, OutOfService::instance());
}

bool ElevatorFsm::Closing::onTimer(ElevatorFsm *fsm)
{
    return changeState(fsm, OutOfService::instance());
}

//---------- Class ElevatorFsm::OutOfService Implementation -------------------

bool ElevatorFsm::OutOfService::enter(ElevatorFsm *fsm)
{
    fsm->ui_.outOfService();
    return true;
}

bool ElevatorFsm::OutOfService::onRestoreService(ElevatorFsm *fsm)
{
    return changeState(fsm, Restoring::instance());
}

//---------- Class ElevatorFsm::Restoring Implementation -------------------

bool ElevatorFsm::Restoring::enter(ElevatorFsm *fsm)
{
    bool result = false;

    // Don't make any assumptions about the elevator position when it was
    // manually returned to service. It could be at a floor or in between
    // floors, at the ground or some other floor. If the elevator is safely
    // at the ground floor, open the door. Otherwise, send it to the ground
    // floor.

    fsm->currentFloor_     = fsm->drive_.getFloor();
    fsm->destinationFloor_ = ElevatorFsm::GROUND_FLOOR;

    fsm->ui_.inService();

    if (fsm->drive_.isAtFloor() &&
        (fsm->currentFloor_== fsm->destinationFloor_))
    {
        result = changeState(fsm, Opening::instance());
    }
    else
    {
        result = changeState(fsm, Moving::instance());
    }

    return result;
}

