// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic_win.h"
#include "device/bluetooth/bluetooth_remote_gatt_service_win.h"
#include "device/bluetooth/bluetooth_socket_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kDeviceName[] = "TestDevice";
const char kDeviceAddress[] = "01:02:03:0A:10:A0";
}

namespace device {
class BluetoothRemoteGattCharacteristicWinTest : public testing::Test {
 public:
  BluetoothRemoteGattCharacteristicWinTest()
      : ui_task_runner_(new base::TestSimpleTaskRunner()),
        bluetooth_task_runner_(new base::TestSimpleTaskRunner()),
        socket_thread_(BluetoothSocketThread::Get()),
        adapter_(new BluetoothAdapterWin(base::Bind(
            &BluetoothRemoteGattCharacteristicWinTest::AdapterInitCallback,
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

    // Create an empty service.
    base::FilePath service_path =
        base::FilePath(std::wstring(L"/bluetooth/test"));
    service_.reset(new BluetoothRemoteGattServiceWin(
        device_.get(), service_path, BluetoothUUID("1234"), 0, true, nullptr,
        service_ui_task_runner_));
    FinishPendingTasks();
  }

  void SetUp() {
    BTH_LE_GATT_CHARACTERISTIC* characteristic_info =
        new BTH_LE_GATT_CHARACTERISTIC[1];
    characteristic_info[0].CharacteristicUuid.IsShortUuid = true;
    characteristic_info[0].CharacteristicUuid.Value.ShortUuid = 4321;
    characteristic_.reset(new BluetoothRemoteGattCharacteristicWin(
        service_.get(), characteristic_info, service_ui_task_runner_));
    FinishPendingTasks();
  }

  void TearDown() override { characteristic_.reset(nullptr); }

  void AdapterInitCallback() {}

  void FinishPendingTasks() {
    bluetooth_task_runner_->RunPendingTasks();
    ui_task_runner_->RunPendingTasks();
  }

  PBTH_LE_GATT_DESCRIPTOR CreateSystemDescriptorInfo(
      std::vector<uint16_t> uuids) {
    PBTH_LE_GATT_DESCRIPTOR descriptors_info =
        new BTH_LE_GATT_DESCRIPTOR[uuids.size()];
    for (unsigned int i = 0; i < uuids.size(); i++) {
      descriptors_info[i].DescriptorUuid.IsShortUuid = true;
      descriptors_info[i].DescriptorUuid.Value.ShortUuid = uuids[i];
    }
    return descriptors_info;
  }

  size_t NumberOfIncludedDescriptors() {
    return characteristic_->included_descriptor_objects_.size();
  }

  void UpdateIncludedDescriptors(PBTH_LE_GATT_DESCRIPTOR sys_decriptors_info,
                                 uint16_t num) {
    characteristic_->GetIncludedDescriptorsCallback(sys_decriptors_info, num,
                                                    S_OK);
  }

  bool IsThisDecriptorIncluded(std::string uuid_string_value) {
    BluetoothRemoteGattCharacteristicWin::GattDescriptorMap::iterator it =
        characteristic_->included_descriptor_objects_.begin();
    for (; it != characteristic_->included_descriptor_objects_.end(); it++) {
      if (it->second->GetUUID().value() == uuid_string_value)
        break;
    }
    if (it != characteristic_->included_descriptor_objects_.end())
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
  scoped_ptr<BluetoothRemoteGattCharacteristicWin> characteristic_;
  scoped_refptr<base::SequencedTaskRunner> service_ui_task_runner_;
};

TEST_F(BluetoothRemoteGattCharacteristicWinTest, UpdateIncludedDescriptors) {
  EXPECT_EQ(NumberOfIncludedDescriptors(), 0);

  // Add number_of_included_desriptors to the Characteristic.
  uint16_t number_of_included_desriptors = 3;
  uint16_t uuids_start_from = 1000;
  std::vector<uint16_t> descriptor_uuids;
  for (uint16_t i = 0; i < number_of_included_desriptors; i++)
    descriptor_uuids.push_back(uuids_start_from + i);
  scoped_ptr<BTH_LE_GATT_DESCRIPTOR> sys_decriptors_info(
      CreateSystemDescriptorInfo(descriptor_uuids));
  UpdateIncludedDescriptors(sys_decriptors_info.get(),
                            number_of_included_desriptors);
  FinishPendingTasks();

  // Check Descriptor objects have been created.
  std::vector<std::string> descriptor_uuids_string_value;
  for (uint16_t i = 0; i < number_of_included_desriptors; i++) {
    std::string uuid_string_value =
        bluetooth_task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
                                   sys_decriptors_info.get()[i].DescriptorUuid)
            .value();
    descriptor_uuids_string_value.push_back(uuid_string_value);
  }
  EXPECT_EQ(NumberOfIncludedDescriptors(), number_of_included_desriptors);
  for (auto uuid_string_value : descriptor_uuids_string_value) {
    EXPECT_TRUE(IsThisDecriptorIncluded(uuid_string_value));
  }

  // Update Descriptors without changing.
  UpdateIncludedDescriptors(sys_decriptors_info.get(),
                            number_of_included_desriptors);
  FinishPendingTasks();

  // Check the included Descriptors have not been changed.
  EXPECT_EQ(NumberOfIncludedDescriptors(), number_of_included_desriptors);
  for (auto uuid_string_value : descriptor_uuids_string_value) {
    EXPECT_TRUE(IsThisDecriptorIncluded(uuid_string_value));
  }

  // Remove a Descriptor.
  descriptor_uuids.erase(descriptor_uuids.begin());
  // Add a new Descriptor.
  descriptor_uuids.push_back(descriptor_uuids.back() + 1);
  sys_decriptors_info.reset(CreateSystemDescriptorInfo(descriptor_uuids));
  UpdateIncludedDescriptors(sys_decriptors_info.get(),
                            number_of_included_desriptors);
  FinishPendingTasks();

  // Check the removed Descriptor's object has been removed.
  EXPECT_FALSE(IsThisDecriptorIncluded(descriptor_uuids_string_value[0]));

  // Check the new and the non removed Descriptors are included.
  descriptor_uuids_string_value.erase(descriptor_uuids_string_value.begin());
  std::string uuid_string_value =
      bluetooth_task_manager_
          ->BluetoothLowEnergyUuidToBluetoothUuid(
              sys_decriptors_info.get()[number_of_included_desriptors - 1]
                  .DescriptorUuid)
          .value();
  descriptor_uuids_string_value.push_back(uuid_string_value);
  EXPECT_EQ(NumberOfIncludedDescriptors(), number_of_included_desriptors);
  for (auto uuid_string_value : descriptor_uuids_string_value) {
    EXPECT_TRUE(IsThisDecriptorIncluded(uuid_string_value));
  }
}

}  // namespace device
