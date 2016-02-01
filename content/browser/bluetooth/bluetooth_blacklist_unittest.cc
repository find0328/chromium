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
  BluetoothUUID non_excluded_uuid("00000000-0000-0000-0000-000000000000");
  EXPECT_FALSE(blacklist.IsExcluded(non_excluded_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromReads(non_excluded_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromWrites(non_excluded_uuid));
}

TEST(BluetoothBlacklistTest, ExcludeUUID) {
  BluetoothBlacklist blacklist;
  BluetoothUUID excluded_uuid("eeeeeeee");
  blacklist.AddOrDie(excluded_uuid, BluetoothBlacklist::Value::EXCLUDE);
  EXPECT_TRUE(blacklist.IsExcluded(excluded_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(excluded_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(excluded_uuid));
}

TEST(BluetoothBlacklistTest, ExcludeReadsUUID) {
  BluetoothBlacklist blacklist;
  BluetoothUUID exclude_reads_uuid("eeeeeeee");
  blacklist.AddOrDie(exclude_reads_uuid, BluetoothBlacklist::Value::EXCLUDE_READS);
  EXPECT_FALSE(blacklist.IsExcluded(exclude_reads_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(exclude_reads_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromWrites(exclude_reads_uuid));
}

TEST(BluetoothBlacklistTest, ExcludeWritesUUID) {
  BluetoothBlacklist blacklist;
  BluetoothUUID exclude_writes_uuid("eeeeeeee");
  blacklist.AddOrDie(exclude_writes_uuid, BluetoothBlacklist::Value::EXCLUDE_WRITES);
  EXPECT_FALSE(blacklist.IsExcluded(exclude_writes_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromReads(exclude_writes_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(exclude_writes_uuid));
}

TEST(BluetoothBlacklistTest, AbreviatedUUIDs) {
  BluetoothBlacklist blacklist;

  blacklist.AddOrDie(BluetoothUUID("aaaa"), BluetoothBlacklist::Value::EXCLUDE);
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("0000aaaa-0000-1000-8000-00805f9b34fb")));

  blacklist.AddOrDie(BluetoothUUID("0000bbbb-0000-1000-8000-00805f9b34fb"), BluetoothBlacklist::Value::EXCLUDE);
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("bbbb")));
}


}  // namespace content
