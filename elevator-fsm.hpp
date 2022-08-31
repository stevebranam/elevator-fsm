// Elevator FSM: An example embedded system FSM using the State pattern,
// unit-testable via Google Test.
//
// The specific FSM model is a Moore machine using the Executable UML (xUML) rules:
// - Moore: Actions are associated with states, not transitions.
// - xUML: Each state has only an entry function, performed whenever the state is entered,
//   including transition-to-self.
//
// References:
// [1] "Design Patterns: Elements of Reusable Object-Oriented Software",
//     by Erich Gamma, Richard Helm, Ralph Johnson, and John Vlissides.
//
// [2] "Executable UML: A Foundation for Model-Driven Architecture",
//     by Stephen Mellor and Marc Balcer.
//
// [3] "Models to Code: With No Mysterious Gaps",
//     by Leon Starr, Andrew Mangogna, and Stephen Mellor.
//
// Author: Steve Branam, sdbranam@gmail.com, December, 2021
//
#ifndef ELEVATOR_FSM_HPP
#define ELEVATOR_FSM_HPP

#include "elevator-fsm-interfaces.hpp"

class ElevatorFsm
    : ElevatorUiClient
    , ElevatorDoorClient
    , ElevatorDriveClient
    , ElevatorTimerClient
{
public:
    ElevatorFsm(
        ElevatorUiApi    &ui,
        ElevatorDoorApi  &door,
        ElevatorDriveApi &drive,
        ElevatorTimerApi &timer);

    enum Floors
    {
        GROUND_FLOOR = 1,
    };

    enum Timers
    {
        TIMEOUT_DOOR_OPEN_MSEC     =  5000,
        TIMEOUT_DOOR_CLOSE_MSEC    =  7000,
        TIMEOUT_MOVE_TO_FLOOR_MSEC = 60000,
        TIMER_WAITING_MSEC         = 10000,
    };

    // Client interface event handlers: store event parameters and forward to FSM event handlers.
    virtual bool handleFloorRequest(size_t floor)
    {
        destinationFloor_ = floor;
        return onFloorRequest();
    }
    virtual bool handleOpenButton()                 { return onOpenButton(); }
    virtual bool handleCloseButton()                { return onCloseButton(); }
    virtual bool handleStopButton()                 { return onStopButton(); }
    virtual bool handleRestoreService()             { return onRestoreService(); }

    virtual bool handleOpened()                     { return onDoorsOpened(); }
    virtual bool handleClosed()                     { return onDoorsClosed(); }
    virtual bool handleDoorFault()                  { return onFault(); }

    virtual bool handleArrived()                    { return onArrived(); }
    virtual bool handleDriveFault()                 { return onFault(); }

    virtual bool handleExpired()                    { return onTimer(); }

    // Is the elevator functioning?
    bool isInService() const;

    // Is the elevator sitting idle at a floor with doors closed?
    bool isIdle() const;

    // Is the elevator waiting at a floor with doors opened?
    bool isWaiting() const;

private:
    class State
    {
    public:
        virtual bool onFloorRequest(ElevatorFsm *fsm) { return false; }
        virtual bool onDoorsOpened(ElevatorFsm *fsm) { return false; }
        virtual bool onDoorsClosed(ElevatorFsm *fsm) { return false; }
        virtual bool onOpenButton(ElevatorFsm *fsm) { return false; }
        virtual bool onCloseButton(ElevatorFsm *fsm) { return false; }
        virtual bool onStopButton(ElevatorFsm *fsm) { return false; }
        virtual bool onRestoreService(ElevatorFsm *fsm) { return false; }
        virtual bool onFault(ElevatorFsm *fsm) { return false; }
        virtual bool onArrived(ElevatorFsm *fsm) { return false; }
        virtual bool onTimer(ElevatorFsm *fsm) { return false; }

    protected:
        bool changeState(ElevatorFsm *fsm, State *newState);

    private:
        virtual bool enter(ElevatorFsm *fsm) { return false; }
    };

    class Stopped
        : public State
    {
    public:
        static Stopped *instance()
        {
            static Stopped me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onFloorRequest(ElevatorFsm *fsm);
        virtual bool onOpenButton(ElevatorFsm *fsm);
    };

    class Moving
        : public State
    {
    public:
        static Moving *instance()
        {
            static Moving me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onArrived(ElevatorFsm *fsm);
        virtual bool onStopButton(ElevatorFsm *fsm);
        virtual bool onFault(ElevatorFsm *fsm);
        virtual bool onTimer(ElevatorFsm *fsm);
    };

    class Holding
        : public State
    {
    public:
        static Holding *instance()
        {
            static Holding me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onStopButton(ElevatorFsm *fsm);
    };

    class Resuming
        : public State
    {
    public:
        static Resuming *instance()
        {
            static Resuming me;
            return &me;
        }

        // Immediately advances state on completion, so no need for additional events.
        virtual bool enter(ElevatorFsm *fsm);
    };

    class Opening
        : public State
    {
    public:
        static Opening *instance()
        {
            static Opening me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onDoorsOpened(ElevatorFsm *fsm);
        virtual bool onFault(ElevatorFsm *fsm);
        virtual bool onTimer(ElevatorFsm *fsm);
    };

    class Waiting
        : public State
    {
    public:
        static Waiting *instance()
        {
            static Waiting me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onOpenButton(ElevatorFsm *fsm);
        virtual bool onCloseButton(ElevatorFsm *fsm);
        virtual bool onTimer(ElevatorFsm *fsm);
    };

    class Closing
        : public State
    {
    public:
        static Closing *instance()
        {
            static Closing me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onDoorsClosed(ElevatorFsm *fsm);
        virtual bool onFault(ElevatorFsm *fsm);
        virtual bool onTimer(ElevatorFsm *fsm);
    };

    class OutOfService
        : public State
    {
    public:
        static OutOfService *instance()
        {
            static OutOfService me;
            return &me;
        }

        virtual bool enter(ElevatorFsm *fsm);
        virtual bool onRestoreService(ElevatorFsm *fsm);
    };

    class Restoring
        : public State
    {
    public:
        static Restoring *instance()
        {
            static Restoring me;
            return &me;
        }

        // Immediately advances state on completion, so no need for additional events.
        virtual bool enter(ElevatorFsm *fsm);
    };


    friend State;
    State *state_;

    // Delegate all events to the current state.
    bool onFloorRequest()   { return state_->onFloorRequest(this); }
    bool onDoorsOpened()    { return state_->onDoorsOpened(this); }
    bool onDoorsClosed()    { return state_->onDoorsClosed(this); }
    bool onOpenButton()     { return state_->onOpenButton(this); }
    bool onCloseButton()    { return state_->onCloseButton(this); }
    bool onStopButton()     { return state_->onStopButton(this); }
    bool onRestoreService() { return state_->onRestoreService(this); }
    bool onFault()          { return state_->onFault(this); }
    bool onArrived()        { return state_->onArrived(this); }
    bool onTimer()          { return state_->onTimer(this); }

    // API's used by this FSM.
    ElevatorUiApi    &ui_;
    ElevatorDoorApi  &door_;
    ElevatorDriveApi &drive_;
    ElevatorTimerApi &timer_;

    size_t currentFloor_;
    size_t destinationFloor_;
};
        
#endif // ELEVATOR_FSM_HPP
