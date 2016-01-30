// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/bluetooth/bluetooth_blacklist.h"

#include "device/bluetooth/bluetooth_uuid.h"
#include "testing/gtest/include/gtest/gtest.h"

using device::BluetoothUUID;

namespace content {

TEST(BluetoothBlacklistTest, MyBluetoothBlacklistTest) {
  BluetoothBlacklist blacklist;
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("00001800-0000-1000-8000-00805f9b34fb")));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(BluetoothUUID("00001800-0000-1000-8000-00805f9b34fb")));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("00001800-0000-1000-8000-00805f9b34fb")));
}

}  // namespace content
