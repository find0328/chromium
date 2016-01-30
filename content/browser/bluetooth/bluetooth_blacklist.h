// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLUETOOTH_BLUETOOTH_BLACKLIST_H_
#define CONTENT_BROWSER_BLUETOOTH_BLUETOOTH_BLACKLIST_H_

#include <map>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace content {

// Implements the Web Bluetooth Blacklist policy as defined in the Web Bluetooth
// specification:
// https://webbluetoothcg.github.io/web-bluetooth/#the-gatt-blacklist
//
// Client code may query UUIDs to determine if they are excluded from use by the
// blacklist.
class CONTENT_EXPORT BluetoothBlacklist final {
 public:
  // Blacklist value terminology from Web Bluetooth specification:
  // https://webbluetoothcg.github.io/web-bluetooth/#the-gatt-blacklist
  enum class Value {
    EXCLUDE,        // Implies EXCLUDE_READS and EXCLUDE_WRITES.
    EXCLUDE_READS,  // Excluded from read operations.
    EXCLUDE_WRITES  // Excluded from write operations.
  };

  BluetoothBlacklist();
  ~BluetoothBlacklist();

  // Returns if an UUID is excluded from all operations.
  bool IsExcluded(const device::BluetoothUUID&) const;

  // Returns if an UUID is excluded from read operations.
  bool IsExcludedFromReads(const device::BluetoothUUID&) const;

  // Returns if an UUID is excluded from write operations.
  bool IsExcludedFromWrites(const device::BluetoothUUID&) const;

 private:
  // Map of UUID to blacklisted value.
  std::map<device::BluetoothUUID, Value> blacklisted_uuids_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothBlacklist);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLUETOOTH_BLUETOOTH_BLACKLIST_H_
