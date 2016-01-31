// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/bluetooth/bluetooth_blacklist.h"

#include "base/logging.h"

using device::BluetoothUUID;

namespace content {

BluetoothBlacklist::BluetoothBlacklist() {
  // Blacklist UUIDs updated 2016-01-30 from:
  // https://github.com/WebBluetoothCG/registries/blob/master/gatt_blacklist.txt
  auto insert_result = blacklisted_uuids_.insert(std::make_pair(
      BluetoothUUID("00001800-0000-1000-8000-00805f9b34fb"), Value::EXCLUDE));
  DCHECK(insert_result.second);  // Assert there was no duplicate.
}

BluetoothBlacklist::~BluetoothBlacklist() {}

bool BluetoothBlacklist::IsExcluded(const BluetoothUUID& uuid) const {
  const auto& it = blacklisted_uuids_.find(uuid);
  if (it == blacklisted_uuids_.end())
    return false;
  return it->second == Value::EXCLUDE;
}

bool BluetoothBlacklist::IsExcludedFromReads(const BluetoothUUID& uuid) const {
  const auto& it = blacklisted_uuids_.find(uuid);
  if (it == blacklisted_uuids_.end())
    return false;
  return it->second == Value::EXCLUDE || it->second == Value::EXCLUDE_READS;
}

bool BluetoothBlacklist::IsExcludedFromWrites(const BluetoothUUID& uuid) const {
  const auto& it = blacklisted_uuids_.find(uuid);
  if (it == blacklisted_uuids_.end())
    return false;
  return it->second == Value::EXCLUDE || it->second == Value::EXCLUDE_WRITES;
}

}  // namespace content
