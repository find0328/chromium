// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/bluetooth_remote_gatt_service_win.h"
#include "device/bluetooth/bluetooth_socket_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kDeviceName[] = "TestDevice";
const char kDeviceAddress[] = "01:02:03:0A:10:A0";
}

namespace device {
class BluetoothRemoteGattServiceWinTest : public testing::Test {
 public:
  BluetoothRemoteGattServiceWinTest()
      : ui_task_runner_(new base::TestSimpleTaskRunner()),
        bluetooth_task_runner_(new base::TestSimpleTaskRunner()),
        socket_thread_(BluetoothSocketThread::Get()),
        adapter_(new BluetoothAdapterWin(
            base::Bind(&BluetoothRemoteGattServiceWinTest::AdapterInitCallback,
                       base::Unretained(this)))) {
    adapter_->InitForTest(ui_task_runner_, bluetooth_task_runner_);

    // Create an empty device.
    BluetoothTaskManagerWin::DeviceState* device_state =
        new BluetoothTaskManagerWin::DeviceState();
    device_state->name = kDeviceName;
    device_state->address = kDeviceAddress;
    device_.reset(new BluetoothDeviceWin(adapter_.get(), *device_state,
                                         ui_task_runner_, socket_thread_,
                                         nullptr, net::NetLog::Source()));
    service_ui_task_runner_ = ui_task_runner_;
    bluetooth_task_manager_ = adapter_->GetWinBluetoothTaskManager();
  }

  void SetUp() override {
    // Create an empty service.
    base::FilePath service_path =
        base::FilePath(std::wstring(L"/bluetooth/test"));
    service_.reset(new BluetoothRemoteGattServiceWin(
        device_.get(), service_path, BluetoothUUID("1234"), 0, true, nullptr,
        service_ui_task_runner_));
    FinishPendingTasks();
  }

  void TearDown() override { service_.reset(nullptr); }

  void AdapterInitCallback() {}

  void FinishPendingTasks() {
    bluetooth_task_runner_->RunPendingTasks();
    ui_task_runner_->RunPendingTasks();
  }

  PBTH_LE_GATT_CHARACTERISTIC CreateSystemGattCharacteristicInfo(
      std::vector<uint16_t> uuids) {
    PBTH_LE_GATT_CHARACTERISTIC characteristics =
        new BTH_LE_GATT_CHARACTERISTIC[uuids.size()];
    for (uint16_t i = 0; i < uuids.size(); i++) {
      characteristics[i].CharacteristicUuid.IsShortUuid = true;
      characteristics[i].CharacteristicUuid.Value.ShortUuid = uuids[i];
    }
    return characteristics;
  }

  PBTH_LE_GATT_SERVICE CreateSystemGattServiceInfo(
      std::vector<uint16_t> uuids) {
    PBTH_LE_GATT_SERVICE services = new BTH_LE_GATT_SERVICE[uuids.size()];
    for (uint16_t i = 0; i < uuids.size(); i++) {
      services[i].ServiceUuid.IsShortUuid = true;
      services[i].ServiceUuid.Value.ShortUuid = uuids[i];
    }
    return services;
  }

  void UpdateIncludedCharacteristics(
      PBTH_LE_GATT_CHARACTERISTIC characteristics,
      uint16_t num) {
    service_->GetIncludedCharacteristicsCallback(characteristics, num, S_OK);
  }

  void UpdateIncludedServices(PBTH_LE_GATT_SERVICE services, uint16_t num) {
    service_->GetIncludedServicesCallback(services, num, S_OK);
  }

  size_t NumberOfIncludedCharacteristics() {
    return service_->included_characteristic_objects_.size();
  }

  size_t NumberOfIncludedServices() {
    return service_->included_service_objects_.size();
  }

  bool IsThisCharacteristicIncluded(std::string uuid_string_value) {
    BluetoothRemoteGattServiceWin::GattCharacteristicsMap::iterator it =
        service_->included_characteristic_objects_.begin();
    for (; it != service_->included_characteristic_objects_.end(); it++) {
      if (it->second->GetUUID().value() == uuid_string_value)
        break;
    }
    if (it != service_->included_characteristic_objects_.end())
      return true;
    return false;
  }

  bool IsThisServiceIncluded(std::string uuid_string_value) {
    BluetoothRemoteGattServiceWin::GattServicesMap::iterator it =
        service_->included_service_objects_.begin();
    for (; it != service_->included_service_objects_.end(); it++) {
      if (it->second->GetUUID().value() == uuid_string_value)
        break;
    }
    if (it != service_->included_service_objects_.end())
      return true;
    return false;
  }

 protected:
  BluetoothTaskManagerWin* bluetooth_task_manager_;

 private:
  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> bluetooth_task_runner_;
  scoped_refptr<BluetoothSocketThread> socket_thread_;

  scoped_refptr<BluetoothAdapterWin> adapter_;
  scoped_ptr<BluetoothDeviceWin> device_;
  scoped_ptr<BluetoothRemoteGattServiceWin> service_;
  scoped_refptr<base::SequencedTaskRunner> service_ui_task_runner_;
};

TEST_F(BluetoothRemoteGattServiceWinTest, UpdateIncludedCharacteristics) {
  EXPECT_EQ(NumberOfIncludedCharacteristics(), 0);

  // Add number_of_characteristics to the service.
  uint16_t number_of_characteristics = 3;
  uint16_t uuid_start_from = 1000;
  std::vector<uint16_t> characteristic_uuids;
  for (uint16_t i = 0; i < number_of_characteristics; i++) {
    characteristic_uuids.push_back(uuid_start_from + i);
  }
  scoped_ptr<BTH_LE_GATT_CHARACTERISTIC> characteristics(
      CreateSystemGattCharacteristicInfo(characteristic_uuids));
  UpdateIncludedCharacteristics(characteristics.get(),
                                number_of_characteristics);
  FinishPendingTasks();

  // Check characteristic objects have been created.
  std::vector<std::string> characteristic_uuid_string_value;
  for (uint16_t i = 0; i < number_of_characteristics; i++) {
    std::string uuid_string_value =
        bluetooth_task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
                                   characteristics.get()[i].CharacteristicUuid)
            .value();
    characteristic_uuid_string_value.push_back(uuid_string_value);
  }
  EXPECT_EQ(NumberOfIncludedCharacteristics(), number_of_characteristics);
  for (auto uuid_string_value : characteristic_uuid_string_value) {
    EXPECT_TRUE(IsThisCharacteristicIncluded(uuid_string_value));
  }

  // Update characteristics without changing.
  UpdateIncludedCharacteristics(characteristics.get(),
                                number_of_characteristics);
  FinishPendingTasks();

  // Check the Characteristics have not been changed.
  EXPECT_EQ(NumberOfIncludedCharacteristics(), number_of_characteristics);
  for (auto uuid_string_value : characteristic_uuid_string_value) {
    EXPECT_TRUE(IsThisCharacteristicIncluded(uuid_string_value));
  }

  // Remove a Characteristic.
  characteristic_uuids.erase(characteristic_uuids.begin());
  // Add a new Characteristic.
  characteristic_uuids.push_back(characteristic_uuids.back() + 1);
  characteristics.reset(
      CreateSystemGattCharacteristicInfo(characteristic_uuids));
  UpdateIncludedCharacteristics(characteristics.get(),
                                number_of_characteristics);
  FinishPendingTasks();

  // Check the removed characteristic object has been removed.
  EXPECT_FALSE(
      IsThisCharacteristicIncluded(characteristic_uuid_string_value[0]));

  // Check the new and the non removed Characteristic objects are there.
  characteristic_uuid_string_value.erase(
      characteristic_uuid_string_value.begin());
  std::string uuid_value =
      bluetooth_task_manager_
          ->BluetoothLowEnergyUuidToBluetoothUuid(
              characteristics.get()[number_of_characteristics - 1]
                  .CharacteristicUuid)
          .value();
  characteristic_uuid_string_value.push_back(uuid_value);
  EXPECT_EQ(NumberOfIncludedCharacteristics(), number_of_characteristics);
  for (auto uuid_string_value : characteristic_uuid_string_value) {
    EXPECT_TRUE(IsThisCharacteristicIncluded(uuid_string_value));
  }
}

TEST_F(BluetoothRemoteGattServiceWinTest, UpdateIncludedServices) {
  EXPECT_EQ(NumberOfIncludedServices(), 0);

  // Add number_of_services to the service.
  uint16_t number_of_services = 3;
  uint16_t uuid_start_from = 1000;
  std::vector<uint16_t> service_uuids;
  for (uint16_t i = 0; i < number_of_services; i++) {
    service_uuids.push_back(uuid_start_from + i);
  }
  scoped_ptr<BTH_LE_GATT_SERVICE> services(
      CreateSystemGattServiceInfo(service_uuids));
  UpdateIncludedServices(services.get(), number_of_services);
  FinishPendingTasks();

  // Check the service objects have been created.
  std::vector<std::string> service_uuid_string_value;
  for (uint16_t i = 0; i < number_of_services; i++) {
    std::string uuid_value =
        bluetooth_task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
                                   services.get()[i].ServiceUuid)
            .value();
    service_uuid_string_value.push_back(uuid_value);
  }
  EXPECT_EQ(NumberOfIncludedServices(), number_of_services);
  for (uint16_t i = 0; i < number_of_services; i++) {
    EXPECT_TRUE(IsThisServiceIncluded(service_uuid_string_value[i]));
  }

  // Update included services without changing.
  UpdateIncludedServices(services.get(), number_of_services);
  FinishPendingTasks();

  // Check the included services haven't been changed.
  EXPECT_EQ(NumberOfIncludedServices(), number_of_services);
  for (uint16_t i = 0; i < number_of_services; i++) {
    EXPECT_TRUE(IsThisServiceIncluded(service_uuid_string_value[i]));
  }

  // Remove a service.
  service_uuids.erase(service_uuids.begin());
  // Add a new service.
  service_uuids.push_back(service_uuids.back() + 1);
  services.reset(CreateSystemGattServiceInfo(service_uuids));
  UpdateIncludedServices(services.get(), number_of_services);
  FinishPendingTasks();

  // Check the removed service's object has been removed.
  EXPECT_FALSE(IsThisServiceIncluded(service_uuid_string_value[0]));

  // Check the new service and the non removed services object are there.
  service_uuid_string_value.erase(service_uuid_string_value.begin());
  std::string uuid_value =
      bluetooth_task_manager_
          ->BluetoothLowEnergyUuidToBluetoothUuid(
              services.get()[number_of_services - 1].ServiceUuid)
          .value();
  service_uuid_string_value.push_back(uuid_value);
  EXPECT_EQ(NumberOfIncludedServices(), number_of_services);
  for (uint16_t i = 0; i < number_of_services; i++) {
    EXPECT_TRUE(IsThisServiceIncluded(service_uuid_string_value[i]));
  }
}

}  // namespace device
