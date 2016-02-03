// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/bluetooth/bluetooth_blacklist.h"

#include "device/bluetooth/bluetooth_uuid.h"
#include "testing/gtest/include/gtest/gtest.h"

using device::BluetoothUUID;

class BluetoothBlacklistTest : public ::testing::Test {
 public:
  BluetoothBlacklistTest() {
    // Because BluetoothBlacklist is used via a singleton instance, the data
    // must be reset for each test.
    content::BluetoothBlacklist::Get().ResetToDefaultValuesForTest();
  }
};

namespace content {

TEST_F(BluetoothBlacklistTest, NonExcludedUUID) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  BluetoothUUID non_excluded_uuid("00000000-0000-0000-0000-000000000000");
  EXPECT_FALSE(blacklist.IsExcluded(non_excluded_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromReads(non_excluded_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromWrites(non_excluded_uuid));
}

TEST_F(BluetoothBlacklistTest, ExcludeUUID) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  BluetoothUUID excluded_uuid("eeeeeeee");
  blacklist.AddOrDie(excluded_uuid, BluetoothBlacklist::Value::EXCLUDE);
  EXPECT_TRUE(blacklist.IsExcluded(excluded_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(excluded_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(excluded_uuid));
}

TEST_F(BluetoothBlacklistTest, ExcludeReadsUUID) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  BluetoothUUID exclude_reads_uuid("eeeeeeee");
  blacklist.AddOrDie(exclude_reads_uuid,
                     BluetoothBlacklist::Value::EXCLUDE_READS);
  EXPECT_FALSE(blacklist.IsExcluded(exclude_reads_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(exclude_reads_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromWrites(exclude_reads_uuid));
}

TEST_F(BluetoothBlacklistTest, ExcludeWritesUUID) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  BluetoothUUID exclude_writes_uuid("eeeeeeee");
  blacklist.AddOrDie(exclude_writes_uuid,
                     BluetoothBlacklist::Value::EXCLUDE_WRITES);
  EXPECT_FALSE(blacklist.IsExcluded(exclude_writes_uuid));
  EXPECT_FALSE(blacklist.IsExcludedFromReads(exclude_writes_uuid));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(exclude_writes_uuid));
}

TEST_F(BluetoothBlacklistTest, AbreviatedUUIDs) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();

  blacklist.AddOrDie(BluetoothUUID("aaaa"), BluetoothBlacklist::Value::EXCLUDE);
  EXPECT_TRUE(blacklist.IsExcluded(
      BluetoothUUID("0000aaaa-0000-1000-8000-00805f9b34fb")));

  blacklist.AddOrDie(BluetoothUUID("0000bbbb-0000-1000-8000-00805f9b34fb"),
                     BluetoothBlacklist::Value::EXCLUDE);
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("bbbb")));
}

TEST_F(BluetoothBlacklistTest, VerifyDefaultBlacklistSize) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  // When adding items to the blacklist the new values should be added in the
  // tests below for each exclusion type.
  EXPECT_EQ(6u, blacklist.size());
}

TEST_F(BluetoothBlacklistTest, VerifyDefaultExcludeList) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("1800")));
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("1801")));
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("1812")));
  EXPECT_TRUE(blacklist.IsExcluded(BluetoothUUID("2a25")));
}

TEST_F(BluetoothBlacklistTest, VerifyDefaultExcludeReadList) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  EXPECT_TRUE(blacklist.IsExcludedFromReads(BluetoothUUID("1800")));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(BluetoothUUID("1801")));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(BluetoothUUID("1812")));
  EXPECT_TRUE(blacklist.IsExcludedFromReads(BluetoothUUID("2a25")));
}

TEST_F(BluetoothBlacklistTest, VerifyDefaultExcludeWriteList) {
  BluetoothBlacklist& blacklist = BluetoothBlacklist::Get();
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("1800")));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("1801")));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("1812")));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("2a25")));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("2902")));
  EXPECT_TRUE(blacklist.IsExcludedFromWrites(BluetoothUUID("2903")));
}

}  // namespace content
