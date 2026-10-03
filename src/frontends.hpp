#pragma once

#include "common.hpp"
#include "xenbus.hpp"
#include "vfb.hpp"
#include "kthread.hpp"
#include "frontend.hpp"
#include "allocator.hpp"

class FrontendManager : protected KThread {
public:
    FrontendManager(StoreHandle &&store) noexcept : _store(wistd::move(store)) {}
    FrontendManager(const FrontendManager &) = delete;
    FrontendManager &operator=(const FrontendManager &) = delete;
    FrontendManager(FrontendManager &&) = delete;
    FrontendManager &operator=(FrontendManager &&) = delete;
    ~FrontendManager();

    NTSTATUS Initialize() noexcept;

protected:
    virtual void ThreadWork() noexcept override;

private:
    NTSTATUS Enumerate() noexcept;

    StoreHandle _store;
    PXENBUS_STORE_WATCH _watch = nullptr;

    UniqueVfb<VfbFrontend> _frontends[VfbMaxChildren];
};
