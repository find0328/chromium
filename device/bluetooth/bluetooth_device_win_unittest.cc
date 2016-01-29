// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/bluetooth_device_win.h"
#include "device/bluetooth/bluetooth_gatt_service.h"
#include "device/bluetooth/bluetooth_service_record_win.h"
#include "device/bluetooth/bluetooth_socket_thread.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kDeviceName[] = "Device";
const char kDeviceAddress[] = "01:02:03:0A:10:A0";

const char kTestAudioSdpName[] = "Audio";
const char kTestAudioSdpBytes[] =
    "35510900000a00010001090001350319110a09000435103506190100090019350619001909"
    "010209000535031910020900093508350619110d090102090100250c417564696f20536f75"
    "726365090311090001";
const device::BluetoothUUID kTestAudioSdpUuid("110a");

const char kTestVideoSdpName[] = "Video";
const char kTestVideoSdpBytes[] =
    "354b0900000a000100030900013506191112191203090004350c3503190100350519000308"
    "0b090005350319100209000935083506191108090100090100250d566f6963652047617465"
    "776179";
const device::BluetoothUUID kTestVideoSdpUuid("1112");

}  // namespace

namespace device {

class BluetoothDeviceWinTest : public testing::Test {
 public:
  BluetoothDeviceWinTest() {
    scoped_refptr<base::SequencedTaskRunner> ui_task_runner(
        new base::TestSimpleTaskRunner());
    scoped_refptr<base::SequencedTaskRunner> bluetooth_task_runner(
        new base::TestSimpleTaskRunner());
    scoped_refptr<BluetoothSocketThread> socket_thread(
        BluetoothSocketThread::Get());

    // Create an adapter.
    adapter_ = new BluetoothAdapterWin(base::Bind(
        &BluetoothDeviceWinTest::AdapterInitCallback, base::Unretained(this)));
    adapter_->InitForTest(ui_task_runner, bluetooth_task_runner);

    // Add device with audio/video services.
    device_state_.reset(new BluetoothTaskManagerWin::DeviceState());
    device_state_->name = kDeviceName;
    device_state_->address = kDeviceAddress;

    BluetoothTaskManagerWin::ServiceRecordState* audio_state =
        new BluetoothTaskManagerWin::ServiceRecordState();
    audio_state->name = kTestAudioSdpName;
    base::HexStringToBytes(kTestAudioSdpBytes, &audio_state->sdp_bytes);
    device_state_->service_record_states.push_back(audio_state);

    BluetoothTaskManagerWin::ServiceRecordState* video_state =
        new BluetoothTaskManagerWin::ServiceRecordState();
    video_state->name = kTestVideoSdpName;
    base::HexStringToBytes(kTestVideoSdpBytes, &video_state->sdp_bytes);
    device_state_->service_record_states.push_back(video_state);

    device_.reset(new BluetoothDeviceWin(adapter_.get(), *device_state_,
                                         ui_task_runner, socket_thread, NULL,
                                         net::NetLog::Source()));

    // Add empty device.
    empty_device_state_.reset(new BluetoothTaskManagerWin::DeviceState());
    empty_device_state_->name = kDeviceName;
    empty_device_state_->address = kDeviceAddress;
    empty_device_.reset(new BluetoothDeviceWin(
        adapter_.get(), *empty_device_state_, ui_task_runner, socket_thread,
        NULL, net::NetLog::Source()));
  }

  void AdapterInitCallback() {}

  void UpdateGattServices(std::vector<BluetoothUUID> service_uuids) {
    BluetoothTaskManagerWin::DeviceState* device_state =
        new BluetoothTaskManagerWin::DeviceState();
    device_state->name = kDeviceName;
    device_state->address = kDeviceAddress;

    BluetoothTaskManagerWin::ServiceRecordState* service_record_state;
    for (unsigned int i = 0; i < service_uuids.size(); i++) {
      service_record_state = new BluetoothTaskManagerWin::ServiceRecordState();
      service_record_state->gatt_uuid = service_uuids[i];
      device_state->service_record_states.push_back(service_record_state);
    }
    empty_device_->UpdateGattServices(*device_state);
  }

  bool IsThisServiceObjectExist(BluetoothUUID uuid) {
    std::vector<BluetoothGattService*> services =
        empty_device_->GetGattServices();
    std::vector<BluetoothGattService*>::iterator it = services.begin();
    for (; it != services.end(); it++) {
      if ((*it)->GetUUID() == uuid)
        break;
    }
    if (it != services.end())
      return true;
    return false;
  }

 protected:
  scoped_refptr<BluetoothAdapterWin> adapter_;
  scoped_ptr<BluetoothDeviceWin> device_;
  scoped_ptr<BluetoothTaskManagerWin::DeviceState> device_state_;
  scoped_ptr<BluetoothDeviceWin> empty_device_;
  scoped_ptr<BluetoothTaskManagerWin::DeviceState> empty_device_state_;
};

TEST_F(BluetoothDeviceWinTest, GetUUIDs) {
  BluetoothDevice::UUIDList uuids = device_->GetUUIDs();

  EXPECT_EQ(2u, uuids.size());
  EXPECT_EQ(kTestAudioSdpUuid, uuids[0]);
  EXPECT_EQ(kTestVideoSdpUuid, uuids[1]);

  uuids = empty_device_->GetUUIDs();
  EXPECT_EQ(0u, uuids.size());
}

TEST_F(BluetoothDeviceWinTest, IsEqual) {
  EXPECT_TRUE(device_->IsEqual(*device_state_));
  EXPECT_FALSE(device_->IsEqual(*empty_device_state_));
  EXPECT_FALSE(empty_device_->IsEqual(*device_state_));
  EXPECT_TRUE(empty_device_->IsEqual(*empty_device_state_));
}

TEST_F(BluetoothDeviceWinTest, GattServiceUpdate) {
  EXPECT_EQ(empty_device_->GetGattServices().size(), 0);

  // Add number_of_test_services to the device.
  uint16_t number_of_test_services = 3;
  uint16_t uuid_value_start_from = 1000;
  std::vector<BluetoothUUID> service_uuids;
  for (uint16_t i = 0; i < number_of_test_services; i++) {
    service_uuids.push_back(
        BluetoothUUID(std::to_string(uuid_value_start_from + i)));
  }
  UpdateGattServices(service_uuids);

  // Check service objects have been created.
  EXPECT_EQ(empty_device_->GetGattServices().size(), number_of_test_services);
  for (auto uuid : service_uuids)
    EXPECT_TRUE(IsThisServiceObjectExist(uuid));

  // Update service without changing.
  UpdateGattServices(service_uuids);

  // Check service objects have not been changed.
  EXPECT_EQ(empty_device_->GetGattServices().size(), number_of_test_services);
  for (auto uuid : service_uuids)
    EXPECT_TRUE(IsThisServiceObjectExist(uuid));

  // Remove one service.
  BluetoothUUID removed_service_uuid = service_uuids[0];
  service_uuids.erase(service_uuids.begin());
  // Add a new service.
  service_uuids.push_back(BluetoothUUID(
      std::to_string(uuid_value_start_from + number_of_test_services)));
  UpdateGattServices(service_uuids);

  // Check remove service's object has been removed.
  EXPECT_FALSE(IsThisServiceObjectExist(removed_service_uuid));

  // Check new service and the other original services are there.
  EXPECT_EQ(empty_device_->GetGattServices().size(), number_of_test_services);
  for (auto uuid : service_uuids)
    EXPECT_TRUE(IsThisServiceObjectExist(uuid));
}

}  // namespace device
