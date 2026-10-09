#pragma once

#include "common.hpp"

class KThread {
public:
    explicit KThread() noexcept;
    KThread(const KThread &) = delete;
    KThread &operator=(const KThread &) = delete;
    KThread(KThread &&) = delete;
    KThread &operator=(KThread &&) = delete;
    virtual ~KThread();

    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS ThreadStart() noexcept;
    void ThreadWake() noexcept {
        KeSetEvent(&_threadEvent, IO_NO_INCREMENT, FALSE);
    }
    void ThreadAlert() noexcept {
        InterlockedExchange(&_threadAlerted, 1);
        ThreadWake();
    }
    _IRQL_requires_(PASSIVE_LEVEL) virtual void ThreadWork() noexcept = 0;
    _IRQL_requires_(PASSIVE_LEVEL) void ThreadJoin() noexcept;

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
