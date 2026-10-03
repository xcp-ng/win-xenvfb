#pragma once

#include "common.hpp"
#include "assert.hpp"

// Note: The lock must live in nonpaged memory and outlive every guard.
class MrswLock {
public:
    class MrswExclusiveGuard;

    class [[nodiscard]] MrswSharedGuard {
    public:
        MrswSharedGuard(const MrswSharedGuard &) = delete;
        MrswSharedGuard &operator=(const MrswSharedGuard &) = delete;
        MrswSharedGuard(MrswSharedGuard &&) = delete;
        MrswSharedGuard &operator=(MrswSharedGuard &&) = delete;

        _IRQL_requires_max_(DISPATCH_LEVEL) ~MrswSharedGuard() {
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
        friend class MrswExclusiveGuard;

        explicit MrswSharedGuard(MrswLock *lock) noexcept : _lock(lock) {}

        MrswLock *_lock;
    };

    class [[nodiscard]] MrswExclusiveGuard {
    public:
        MrswExclusiveGuard(const MrswExclusiveGuard &) = delete;
        MrswExclusiveGuard &operator=(const MrswExclusiveGuard &) = delete;
        MrswExclusiveGuard(MrswExclusiveGuard &&) = delete;
        MrswExclusiveGuard &operator=(MrswExclusiveGuard &&) = delete;

        _IRQL_requires_max_(APC_LEVEL) ~MrswExclusiveGuard() {
            if (_lock) {
                ExReInitializeRundownProtection(&_lock->_rundown);
                KeReleaseGuardedMutex(&_lock->_mutex);
                _lock = nullptr;
            }
        }

        explicit operator bool() const noexcept {
            return _lock != nullptr;
        }

        [[nodiscard]] _IRQL_requires_max_(APC_LEVEL) MrswSharedGuard Downgrade() noexcept {
            BUG_ON(!_lock);

            auto lock = _lock;
            ExReInitializeRundownProtection(&lock->_rundown);
            BUG_ON(!ExAcquireRundownProtection(&lock->_rundown));
            KeReleaseGuardedMutex(&lock->_mutex);
            _lock = nullptr;
            return MrswSharedGuard(lock);
        }

    private:
        friend class MrswLock;

        explicit MrswExclusiveGuard(MrswLock *lock) noexcept : _lock(lock) {}

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

    [[nodiscard]] _IRQL_requires_max_(APC_LEVEL) MrswSharedGuard AcquireShared() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, APC_LEVEL);

        if (ExAcquireRundownProtection(&_rundown)) {
            return MrswSharedGuard(this);
        }

        KeAcquireGuardedMutex(&_mutex);
        BUG_ON(!ExAcquireRundownProtection(&_rundown));
        KeReleaseGuardedMutex(&_mutex);
        return MrswSharedGuard(this);
    }

    [[nodiscard]] _IRQL_requires_max_(DISPATCH_LEVEL) MrswSharedGuard TryAcquireShared() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, DISPATCH_LEVEL);
        return MrswSharedGuard(ExAcquireRundownProtection(&_rundown) ? this : nullptr);
    }

    [[nodiscard]] _IRQL_requires_max_(DISPATCH_LEVEL) MrswSharedGuard SpinAcquireShared() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, DISPATCH_LEVEL);
        while (!ExAcquireRundownProtection(&_rundown)) {
            YieldProcessor();
        }
        return MrswSharedGuard(this);
    }

    [[nodiscard]] _IRQL_requires_max_(APC_LEVEL) MrswExclusiveGuard AcquireExclusive() noexcept {
        ASSERT3U(KeGetCurrentIrql(), <=, APC_LEVEL);

        KeAcquireGuardedMutex(&_mutex);
        ExWaitForRundownProtectionRelease(&_rundown);
        ExRundownCompleted(&_rundown);
        return MrswExclusiveGuard(this);
    }

private:
    KGUARDED_MUTEX _mutex{};
    EX_RUNDOWN_REF _rundown{};
};

using MrswSharedLockGuard = MrswLock::MrswSharedGuard;
using MrswExclusiveLockGuard = MrswLock::MrswExclusiveGuard;
