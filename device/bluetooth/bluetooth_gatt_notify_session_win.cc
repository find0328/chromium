// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_gatt_notify_session_win.h"

namespace device {
BluetoothGattNotifySessionWin::BluetoothGattNotifySessionWin(
    base::WeakPtr<BluetoothRemoteGattCharacteristicWin> characteristic)
    : is_active_(true), characteristic_(characteristic) {}

BluetoothGattNotifySessionWin::~BluetoothGattNotifySessionWin() {
  if (characteristic_.get() != NULL && is_active_)
    characteristic_.get()->StopNotifySession();
}

std::string BluetoothGattNotifySessionWin::GetCharacteristicIdentifier() const {
  if (characteristic_.get() != NULL)
    return characteristic_.get()->GetIdentifier();
  return std::string();
}

bool BluetoothGattNotifySessionWin::IsActive() {
  return is_active_ && characteristic_.get() != NULL;
}

void BluetoothGattNotifySessionWin::Stop(const base::Closure& callback) {
  is_active_ = false;
  if (characteristic_.get() != NULL)
    return characteristic_.get()->StopNotifySession();
  callback.Run();
}
}
