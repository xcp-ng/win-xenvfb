#pragma once

#include "common.hpp"
#include "assert.hpp"

// Note: The lock must live in nonpaged memory and outlive every guard.
class MrswLock {
public:
    class ExclusiveGuard;

    class [[nodiscard]] SharedGuard {
    public:
        SharedGuard(const SharedGuard &) = delete;
        SharedGuard &operator=(const SharedGuard &) = delete;
        SharedGuard(SharedGuard &&) = delete;
        SharedGuard &operator=(SharedGuard &&) = delete;

        _IRQL_requires_max_(DISPATCH_LEVEL) ~SharedGuard() {
            if (_lock) {
                ExReleaseRundownProtection(&_lock->_rundown);
                _lock = nullptr;
            }
        }

        explicit operator bool() const noexcept {
            return _lock != nullptr;
        }

    private:
        friend class MrswLock;
        friend class ExclusiveGuard;

        explicit SharedGuard(MrswLock *lock) noexcept : _lock(lock) {}

        MrswLock *_lock;
    };

    class [[nodiscard]] ExclusiveGuard {
    public:
        ExclusiveGuard(const ExclusiveGuard &) = delete;
        ExclusiveGuard &operator=(const ExclusiveGuard &) = delete;
        ExclusiveGuard(ExclusiveGuard &&) = delete;
        ExclusiveGuard &operator=(ExclusiveGuard &&) = delete;

        _IRQL_requires_max_(APC_LEVEL) ~ExclusiveGuard() {
            if (_lock) {
                ExReInitializeRundownProtection(&_lock->_rundown);
                KeReleaseGuardedMutex(&_lock->_mutex);
                _lock = nullptr;
            }
        }

        explicit operator bool() const noexcept {
            return _lock != nullptr;
        }

        [[nodiscard]] _IRQL_requires_max_(APC_LEVEL) SharedGuard Downgrade() noexcept {
            BUG_ON(!_lock);

            auto lock = _lock;
            ExReInitializeRundownProtection(&lock->_rundown);
            BUG_ON(!ExAcquireRundownProtection(&lock->_rundown));
            KeReleaseGuardedMutex(&lock->_mutex);
            _lock = nullptr;
            return SharedGuard(lock);
        }

    private:
        friend class MrswLock;

        explicit ExclusiveGuard(MrswLock *lock) noexcept : _lock(lock) {}

        MrswLock *_lock;
    };

    _IRQL_requires_max_(APC_LEVEL) MrswLock() noexcept {
        KeInitializeGuardedMutex(&_mutex);
        ExInitializeRundownProtection(&_rundown);
    }

    MrswLock(const MrswLock &) = delete;
    MrswLock &operator=(const MrswLock &) = delete;
    MrswLock(MrswLock &&) = delete;
    MrswLock &operator=(MrswLock &&) = delete;

    _IRQL_requires_max_(APC_LEVEL) ~MrswLock() {
#if DBG
        BUG_ON(!KeTryToAcquireGuardedMutex(&_mutex));
        BUG_ON(!ExAcquireRundownProtection(&_rundown));
        KeReleaseGuardedMutex(&_mutex);
        ExReleaseRundownProtection(&_rundown);
#endif
    }

    [[nodiscard]] _IRQL_requires_max_(APC_LEVEL) SharedGuard AcquireShared() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, APC_LEVEL);

        if (ExAcquireRundownProtection(&_rundown)) {
            return SharedGuard(this);
        }

        KeAcquireGuardedMutex(&_mutex);
        BUG_ON(!ExAcquireRundownProtection(&_rundown));
        KeReleaseGuardedMutex(&_mutex);
        return SharedGuard(this);
    }

    [[nodiscard]] _IRQL_requires_max_(DISPATCH_LEVEL) SharedGuard TryAcquireShared() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, DISPATCH_LEVEL);
        return SharedGuard(ExAcquireRundownProtection(&_rundown) ? this : nullptr);
    }

    [[nodiscard]] _IRQL_requires_max_(DISPATCH_LEVEL) SharedGuard SpinAcquireShared() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, DISPATCH_LEVEL);
        while (!ExAcquireRundownProtection(&_rundown)) {
            YieldProcessor();
        }
        return SharedGuard(this);
    }

    [[nodiscard]] _IRQL_requires_max_(APC_LEVEL) ExclusiveGuard AcquireExclusive() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, APC_LEVEL);

        KeAcquireGuardedMutex(&_mutex);
        ExWaitForRundownProtectionRelease(&_rundown);
        ExRundownCompleted(&_rundown);
        return ExclusiveGuard(this);
    }

private:
    KGUARDED_MUTEX _mutex{};
    EX_RUNDOWN_REF _rundown{};
};

using MrswSharedLockGuard = MrswLock::SharedGuard;
using MrswExclusiveLockGuard = MrswLock::ExclusiveGuard;
