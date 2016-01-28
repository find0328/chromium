// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLUETOOTH_BLUETOOTH_BLACKLIST_H_
#define CONTENT_BROWSER_BLUETOOTH_BLUETOOTH_BLACKLIST_H_

#include <map>

#include "device/bluetooth/bluetooth_uuid.h"

namespace content {

// Implements the Web Bluetooth Blacklist policy as defined in the Web Bluetooth
// specification:
// https://webbluetoothcg.github.io/web-bluetooth/#the-gatt-blacklist
//
// Client code may query UUIDs to determine if they are valid to be used.
class BluetoothBlacklist final {
 public:
  struct Value {
    bool exclude_read : 1;
    bool exclude_write : 1;
  }

  // Blacklist value terminology from Web Bluetooth specification:
  // https://webbluetoothcg.github.io/web-bluetooth/#the-gatt-blacklist
  enum class Value {
    EXCLUDE,        // Implies EXCLUDE_READS and EXCLUDE_WRITES.
    EXCLUDE_READS,  // Excluded from read operations.
    EXCLUDE_WRITES  // Excluded from write operations.
  };

  BluetoothBlacklist();

  // Returns if an UUID is excluded from all operations.
  bool Excluded(const BluetoothUUID&);

  // Returns if an UUID is excluded from read operations.
  bool ExcludedFromReads(const BluetoothUUID&);

  // Returns if an UUID is excluded from write operations.
  bool ExcludedFromWrites(const BluetoothUUID&);

 private:
  // Map of UUID to blacklisted value.
  std::map<device::BluetoothUUID, Value> blacklisted_uuids_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLUETOOTH_BLUETOOTH_BLACKLIST_H_
