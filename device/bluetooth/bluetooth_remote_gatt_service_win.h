// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_SERVICE_WIN_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_SERVICE_WIN_H_

#include "base/macros.h"
#include "base/files/file.h"
#include "device/bluetooth/bluetooth_adapter_win.h"
#include "device/bluetooth/bluetooth_device_win.h"
#include "device/bluetooth/bluetooth_gatt_service.h"
#include "device/bluetooth/bluetooth_low_energy_defs_win.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic_win.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"

namespace device {

// The BluetoothRemoteGattServiceWin class implements BluetoothGattService
// for remote GATT services on Windows 8 and later.
class DEVICE_BLUETOOTH_EXPORT BluetoothRemoteGattServiceWin
    : public BluetoothGattService {
 public:
  BluetoothRemoteGattServiceWin(
      BluetoothDeviceWin* device,
      base::FilePath service_path,
      BluetoothUUID service_uuid,
      uint16_t service_attribute_handle,
      bool is_primary,
      BluetoothRemoteGattServiceWin* parent_service,
      scoped_refptr<base::SequencedTaskRunner>& ui_task_runner);
  ~BluetoothRemoteGattServiceWin() override;

  // Override BluetoothGattService interfaces.
  std::string GetIdentifier() const override;
  BluetoothUUID GetUUID() const override;
  bool IsLocal() const override;
  bool IsPrimary() const override;
  BluetoothDevice* GetDevice() const override;
  std::vector<BluetoothGattCharacteristic*> GetCharacteristics() const override;
  std::vector<BluetoothGattService*> GetIncludedServices() const override;
  BluetoothGattCharacteristic* GetCharacteristic(
      const std::string& identifier) const override;
  bool AddCharacteristic(BluetoothGattCharacteristic* characteristic) override;
  bool AddIncludedService(BluetoothGattService* service) override;
  void Register(const base::Closure& callback,
                const ErrorCallback& error_callback) override;
  void Unregister(const base::Closure& callback,
                  const ErrorCallback& error_callback) override;

  // Called by included services to notify it is ready.
  void NotifyGattServiceAdded(BluetoothRemoteGattServiceWin* service);
  // Called by included characteristic to notify it is ready.
  void NotifyGattCharacteristicAdded(
      BluetoothRemoteGattCharacteristicWin* characteristic);

  // Update included services and characteristics.
  void Update();

  BluetoothAdapterWin* GetAdapter() { return adapter_; }
  base::FilePath GetServicePath() { return service_path_; }

 private:
  friend class BluetoothRemoteGattServiceWinTest;

  void GetIncludedCharacteristicsCallback(
      PBTH_LE_GATT_CHARACTERISTIC characteristics,
      uint16_t num,
      HRESULT hr);
  void GetIncludedServicesCallback(PBTH_LE_GATT_SERVICE services,
                                   uint16_t num,
                                   HRESULT hr);
  void UpdateIncludedCharacteristics(
      PBTH_LE_GATT_CHARACTERISTIC characteristics,
      uint16_t num);
  void UpdateIncludedServices(PBTH_LE_GATT_SERVICE services, uint16_t num);
  void NotifyServiceDiscComplIfNecessary();

  BluetoothAdapterWin* adapter_;
  BluetoothDeviceWin* device_;
  base::FilePath service_path_;
  BluetoothUUID service_uuid_;
  uint16_t service_attribute_handle_;
  bool is_primary_;
  BluetoothRemoteGattServiceWin* parent_service_;

  scoped_refptr<BluetoothTaskManagerWin> task_manager_;

  // Flag to indicate service added notification has been send out.
  bool complete_notified_;

  std::set<std::string> discovery_completed_included_charateristics_;
  std::set<std::string> discovery_completed_included_services_;
  bool included_services_discovered_;
  bool included_characteristics_discovered_;

  typedef base::ScopedPtrHashMap<
      std::string,
      scoped_ptr<BluetoothRemoteGattCharacteristicWin>> GattCharacteristicsMap;
  GattCharacteristicsMap included_characteristic_objects_;

  typedef base::ScopedPtrHashMap<std::string,
                                 scoped_ptr<BluetoothRemoteGattServiceWin>>
      GattServicesMap;
  GattServicesMap included_service_objects_;

  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;

  base::WeakPtrFactory<BluetoothRemoteGattServiceWin> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BluetoothRemoteGattServiceWin);
};

}  // namespace device.
#endif  // DEVICE_BLUETOOTH_BLUETOOTH_REMOTE_GATT_SERVICE_WIN_H_
