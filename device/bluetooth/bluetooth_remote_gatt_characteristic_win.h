// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_CHARACTERISTIC_WIN_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_CHARACTERISTIC_WIN_H_

#include "base/macros.h"
#include "device/bluetooth/bluetooth_adapter_win.h"
#include "device/bluetooth/bluetooth_device_win.h"
#include "device/bluetooth/bluetooth_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor_win.h"
#include "device/bluetooth/bluetooth_remote_gatt_service_win.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"

namespace device {

// The BluetoothRemoteGattCharacteristicWin class implements
// BluetoothGattCharacteristic for remote GATT services on Windows 8 and later.
class DEVICE_BLUETOOTH_EXPORT BluetoothRemoteGattCharacteristicWin
    : public BluetoothGattCharacteristic {
 public:
  BluetoothRemoteGattCharacteristicWin(
      BluetoothRemoteGattServiceWin* parent_service,
      BTH_LE_GATT_CHARACTERISTIC* characteristic_info,
      scoped_refptr<base::SequencedTaskRunner>& ui_task_runner);
  ~BluetoothRemoteGattCharacteristicWin();

  // Override BluetoothGattCharacteristic interfaces.
  std::string GetIdentifier() const override;
  BluetoothUUID GetUUID() const override;
  bool IsLocal() const override;
  std::vector<uint8_t>& GetValue() const override;
  BluetoothGattService* GetService() const override;
  Properties GetProperties() const override;
  Permissions GetPermissions() const override;
  bool IsNotifying() const override;
  std::vector<BluetoothGattDescriptor*> GetDescriptors() const override;
  BluetoothGattDescriptor* GetDescriptor(
      const std::string& identifier) const override;
  bool AddDescriptor(BluetoothGattDescriptor* descriptor) override;
  bool UpdateValue(const std::vector<uint8_t>& value) override;
  void StartNotifySession(const NotifySessionCallback& callback,
                          const ErrorCallback& error_callback) override;
  void ReadRemoteCharacteristic(const ValueCallback& callback,
                                const ErrorCallback& error_callback) override;
  void WriteRemoteCharacteristic(const std::vector<uint8_t>& new_value,
                                 const base::Closure& callback,
                                 const ErrorCallback& error_callback) override;

  void NotifyGattDescriptorAdded(BluetoothRemoteGattDescriptorWin* descriptor);
  void OnRemoteCharacteristicValueChanged(BTH_LE_GATT_EVENT_TYPE type,
                                          PVOID event_parameter);
  void StopNotifySession();
  void Update();

 private:
  friend class BluetoothRemoteGattCharacteristicWinTest;

  void NotifyCharacteristicDiscComplIfNecessary();
  void NotifyGattCharacteristicValueChanged(uint8_t* new_value, ULONG size);
  void GetIncludedDescriptorsCallback(PBTH_LE_GATT_DESCRIPTOR descriptors,
                                      uint16_t num,
                                      HRESULT hr);
  void ReadCharacteristicValueCallback(PBTH_LE_GATT_CHARACTERISTIC_VALUE value,
                                       HRESULT hr);
  void WriteRemoteCharacteristicCallback(HRESULT hr);
  void GattEventRegistrationCallback(BLUETOOTH_GATT_EVENT_HANDLE event_handle,
                                     HRESULT hr);
  void UpdateIncludedDescriptors(PBTH_LE_GATT_DESCRIPTOR descriptors,
                                 uint16_t num);

  BluetoothAdapterWin* adapter_;
  BluetoothRemoteGattServiceWin* parent_service_;
  scoped_refptr<BluetoothTaskManagerWin> task_manager_;

  // Characteristic info from OS and used to interact with the OS later.
  scoped_ptr<BTH_LE_GATT_CHARACTERISTIC> characteristic_info_;
  BluetoothUUID characteristic_uuid_;

  bool characteristic_value_initialized_;
  std::vector<uint8_t> characteristic_value_;

  // Flag to indicate NotifyGattCharacteristicAdded has been called.
  bool complete_notified_;

  typedef base::ScopedPtrHashMap<std::string,
                                 scoped_ptr<BluetoothRemoteGattDescriptorWin>>
      GattDescriptorMap;
  GattDescriptorMap included_descriptor_objects_;
  // Contain discovery completed included descriptors' identifier.
  std::set<std::string> completed_descriptors_;
  // Flag to indicate included descriptors have been discovered.
  bool descriptor_discovered_;

  std::vector<std::pair<ValueCallback, ErrorCallback>>
      read_remote_characteristic_value_callbacks_;
  std::vector<std::pair<base::Closure, ErrorCallback>>
      write_remote_characteristic_value_callback_;

  // Flag to indicate whether remote value change notification has been
  // registered.
  bool is_notifying_;
  int number_of_active_notify_sessions_;
  std::vector<std::pair<NotifySessionCallback, ErrorCallback>>
      start_notifying_callback_;
  BLUETOOTH_GATT_EVENT_HANDLE registered_event_handle_;

  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;

  base::WeakPtrFactory<BluetoothRemoteGattCharacteristicWin> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BluetoothRemoteGattCharacteristicWin);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_CHARACTERISTIC_WIN_H_
