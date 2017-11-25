// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/lock_impl.h"

#include "base/debug/activity_tracker.h"

namespace base {
namespace internal {

/// youxiao patch
#if 0
LockImpl::LockImpl() : native_handle_(SRWLOCK_INIT) {}

LockImpl::~LockImpl() = default;
#else
LockImpl::LockImpl()  {
  ::InitializeCriticalSectionAndSpinCount(&native_handle_, 2000);
}

LockImpl::~LockImpl() {
  ::DeleteCriticalSection(&native_handle_);
}
#endif
///

bool LockImpl::Try() {
  /// youxiao patch
  #if 0
  return !!::TryAcquireSRWLockExclusive(&native_handle_);
  #else
  return ::TryEnterCriticalSection(&native_handle_);
  #endif
  ///
}

void LockImpl::Lock() {
  /// youxiao patch
  #if 0
  base::debug::ScopedLockAcquireActivity lock_activity(this);
  ::AcquireSRWLockExclusive(&native_handle_);
  #else
  ::EnterCriticalSection(&native_handle_);
  #endif
  ///
}

void LockImpl::Unlock() {
  /// youxiao patch
  #if 0
  ::ReleaseSRWLockExclusive(&native_handle_);
  #else
  ::LeaveCriticalSection(&native_handle_);
  #endif
  ///
}

}  // namespace internal
}  // namespace base
