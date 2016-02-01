// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/bluetooth/bluetooth_blacklist.h"

#include "device/bluetooth/bluetooth_uuid.h"
#include "testing/gtest/include/gtest/gtest.h"

using device::BluetoothUUID;

namespace content {

TEST(BluetoothBlacklistTest, NonExcludedUUID) {
  BluetoothBlacklist blacklist;
  BluetoothUUID non_excluded_UUID("00000000-0000-0000-0000-000000000000");
  EXPECT_FALSE(blacklist.IsExcluded(non_excluded_UUID));
  EXPECT_FALSE(blacklist.IsExcludedFromReads(non_excluded_UUID));
  EXPECT_FALSE(blacklist.IsExcludedFromWrites(non_excluded_UUID));
}

TEST(BluetoothBlacklistTest, ExcludeUUID) {
  BluetoothBlacklist blacklist;
  BluetoothUUID excluded_UUID("00001800-0000-1000-8000-00805f9b34fb");
  EXPECT_TRUE(blacklist.IsExcluded(excluded_UUID));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(excluded_UUID));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(excluded_UUID));
}

TEST(BluetoothBlacklistTest, ExcludeWritesUUID) {
  BluetoothBlacklist blacklist;
  BluetoothUUID exclude_writes_UUID("00002902-0000-1000-8000-00805f9b34fb");
  EXPECT_FALSE(blacklist.IsExcluded(exclude_writes_UUID));
  EXPECT_FALSE(blacklist.IsExcludedFromReads(exclude_writes_UUID));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(exclude_writes_UUID));
}

}  // namespace content
