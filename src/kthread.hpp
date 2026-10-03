#pragma once

#include "common.hpp"

class KThread {
public:
    KThread() noexcept;
    KThread(const KThread &) = delete;
    KThread &operator=(const KThread &) = delete;
    KThread(KThread &&) = delete;
    KThread &operator=(KThread &&) = delete;
    virtual ~KThread();

    NTSTATUS ThreadStart() noexcept;
    void ThreadWake() noexcept {
        KeSetEvent(&_threadEvent, IO_NO_INCREMENT, FALSE);
    }
    void ThreadAlert() noexcept {
        InterlockedExchange(&_threadAlerted, 1);
        ThreadWake();
    }
    virtual void ThreadWork() noexcept = 0;
    void ThreadJoin() noexcept;

protected:
    bool ThreadIsAlerted() noexcept {
        return ReadAcquire(&_threadAlerted) != 0;
    }

    KEVENT _threadEvent{};

private:
    PKTHREAD _thread = nullptr;
    LONG _threadAlerted = 0;
    bool _threadJoined = false;
};
