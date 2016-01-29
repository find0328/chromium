// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_DESCRIPTOR_WIN_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_DESCRIPTOR_WIN_H_

#include "base/macros.h"
#include "device/bluetooth/bluetooth_adapter_win.h"
#include "device/bluetooth/bluetooth_device_win.h"
#include "device/bluetooth/bluetooth_gatt_descriptor.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic_win.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"

namespace device {

// The BluetoothRemoteGattDescriptorWin class implements BluetoothGattDescriptor
// for remote GATT services on Windows 8 and later.
class DEVICE_BLUETOOTH_EXPORT BluetoothRemoteGattDescriptorWin
    : public BluetoothGattDescriptor {
 public:
  BluetoothRemoteGattDescriptorWin(
      BluetoothAdapterWin* adapter,
      base::FilePath service_path,
      BluetoothRemoteGattCharacteristicWin* parent_characteristic,
      BTH_LE_GATT_DESCRIPTOR* descriptor_info,
      scoped_refptr<base::SequencedTaskRunner>& ui_task_runner);
  ~BluetoothRemoteGattDescriptorWin();

  // Override BluetoothGattDescriptor interfaces.
  std::string GetIdentifier() const override;
  BluetoothUUID GetUUID() const override;
  bool IsLocal() const override;
  std::vector<uint8_t>& GetValue() const override;
  BluetoothGattCharacteristic* GetCharacteristic() const override;
  BluetoothGattCharacteristic::Permissions GetPermissions() const override;
  void ReadRemoteDescriptor(const ValueCallback& callback,
                            const ErrorCallback& error_callback) override;
  void WriteRemoteDescriptor(const std::vector<uint8_t>& new_value,
                             const base::Closure& callback,
                             const ErrorCallback& error_callback) override;

  void Update();

 private:
  void ReadRemoteDescriptorValueCallback(BTH_LE_GATT_DESCRIPTOR_VALUE* value,
                                         HRESULT hr);
  void WriteRemoteDescriptorValueCallback(HRESULT hr);
  BluetoothGattService::GattErrorCode SystemErrorToGattErrorCode(HRESULT hr);

  BluetoothAdapterWin* adapter_;
  base::FilePath service_path_;
  scoped_refptr<BluetoothTaskManagerWin> task_manager_;

  BluetoothRemoteGattCharacteristicWin* parent_characteristic_;
  scoped_ptr<BTH_LE_GATT_DESCRIPTOR> descriptor_info_;
  BluetoothGattCharacteristic::Permissions permissions_;

  BluetoothUUID descriptor_uuid_;
  bool descriptor_initialized_;
  std::vector<uint8_t> descriptor_value_;

  std::vector<std::pair<ValueCallback, ErrorCallback>>
      read_remote_descriptor_value_callbacks_;
  std::vector<std::pair<base::Closure, ErrorCallback>>
      write_remote_descriptor_value_callbacks_;

  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;

  base::WeakPtrFactory<BluetoothRemoteGattDescriptorWin> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BluetoothRemoteGattDescriptorWin);
};

}  // namespace device.
#endif  // DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_DESCRIPTOR_WIN_H_
