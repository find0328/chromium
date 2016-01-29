// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_remote_gatt_descriptor_win.h"

#include "base/bind.h"

namespace device {

BluetoothRemoteGattDescriptorWin::BluetoothRemoteGattDescriptorWin(
    BluetoothAdapterWin* adapter,
    base::FilePath service_path,
    BluetoothRemoteGattCharacteristicWin* parent_characteristic,
    BTH_LE_GATT_DESCRIPTOR* descriptor_info,
    scoped_refptr<base::SequencedTaskRunner>& ui_task_runner)
    : adapter_(adapter),
      service_path_(service_path),
      parent_characteristic_(parent_characteristic),
      descriptor_info_(descriptor_info),
      ui_task_runner_(ui_task_runner),
      descriptor_initialized_(false),
      weak_ptr_factory_(this) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  descriptor_value_.clear();
  read_remote_descriptor_value_callbacks_.clear();
  write_remote_descriptor_value_callbacks_.clear();

  task_manager_ = adapter_->GetWinBluetoothTaskManager();
  DCHECK(parent_characteristic_);
  DCHECK(descriptor_info_.get());
  DCHECK(adapter_);
  DCHECK(task_manager_);

  descriptor_uuid_ = task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
      descriptor_info_.get()->DescriptorUuid);
  // TODO(gogerald): Could not find permission information from
  // BTH_LE_GATT_DESCRIPTOR. Give read and write permission by default. Update
  // it according to future read/write feedback. Use system information when it
  // is available.
  permissions_ = BluetoothGattCharacteristic::PERMISSION_READ |
                 BluetoothGattCharacteristic::PERMISSION_WRITE;
  Update();
}

BluetoothRemoteGattDescriptorWin::~BluetoothRemoteGattDescriptorWin() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  adapter_->NotifyGattDescriptorRemoved(this);
}

std::string BluetoothRemoteGattDescriptorWin::GetIdentifier() const {
  return parent_characteristic_->GetIdentifier() + "_" +
         descriptor_uuid_.value();
}

BluetoothUUID BluetoothRemoteGattDescriptorWin::GetUUID() const {
  return descriptor_uuid_;
}

bool BluetoothRemoteGattDescriptorWin::IsLocal() const {
  return false;
}

std::vector<uint8_t>& BluetoothRemoteGattDescriptorWin::GetValue() const {
  return const_cast<std::vector<uint8_t>&>(descriptor_value_);
}

BluetoothGattCharacteristic*
BluetoothRemoteGattDescriptorWin::GetCharacteristic() const {
  return parent_characteristic_;
}

BluetoothGattCharacteristic::Permissions
BluetoothRemoteGattDescriptorWin::GetPermissions() const {
  return permissions_;
}

BluetoothGattService::GattErrorCode
BluetoothRemoteGattDescriptorWin::SystemErrorToGattErrorCode(HRESULT hr) {
  if (hr == E_BLUETOOTH_ATT_READ_NOT_PERMITTED ||
      hr == E_BLUETOOTH_ATT_WRITE_NOT_PERMITTED) {
    return BluetoothGattService::GATT_ERROR_NOT_PERMITTED;
  }
  return BluetoothGattService::GATT_ERROR_FAILED;
}

void BluetoothRemoteGattDescriptorWin::ReadRemoteDescriptorValueCallback(
    BTH_LE_GATT_DESCRIPTOR_VALUE* value,
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed at reading descriptor value";
    BluetoothGattService::GattErrorCode error = SystemErrorToGattErrorCode(hr);
    if (error == BluetoothGattService::GATT_ERROR_NOT_PERMITTED) {
      permissions_ =
          permissions_ & ~BluetoothGattCharacteristic::PERMISSION_READ;
    }
    for (const auto callback : read_remote_descriptor_value_callbacks_)
      callback.second.Run(error);
  } else {
    descriptor_value_.clear();
    for (ULONG i = 0; i < value->DataSize; i++)
      descriptor_value_.push_back(value->Data[i]);
    for (const auto callback : read_remote_descriptor_value_callbacks_)
      callback.first.Run(descriptor_value_);
  }

  if (!descriptor_initialized_) {
    parent_characteristic_->NotifyGattDescriptorAdded(this);
    descriptor_initialized_ = true;
  }
  read_remote_descriptor_value_callbacks_.clear();
}

void BluetoothRemoteGattDescriptorWin::ReadRemoteDescriptor(
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  read_remote_descriptor_value_callbacks_.push_back(
      std::make_pair(callback, error_callback));
  task_manager_->PostReadDescriptorValue(
      service_path_, descriptor_info_.get(),
      base::Bind(
          &BluetoothRemoteGattDescriptorWin::ReadRemoteDescriptorValueCallback,
          weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothRemoteGattDescriptorWin::WriteRemoteDescriptorValueCallback(
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr)) {
    BluetoothGattService::GattErrorCode error = SystemErrorToGattErrorCode(hr);
    if (error == BluetoothGattService::GATT_ERROR_NOT_PERMITTED) {
      permissions_ =
          permissions_ & ~BluetoothGattCharacteristic::PERMISSION_WRITE;
    }
    for (const auto callback : write_remote_descriptor_value_callbacks_)
      callback.second.Run(error);
  } else {
    for (auto callback : write_remote_descriptor_value_callbacks_)
      callback.first.Run();
  }

  write_remote_descriptor_value_callbacks_.clear();
}

void BluetoothRemoteGattDescriptorWin::WriteRemoteDescriptor(
    const std::vector<uint8_t>& new_value,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  write_remote_descriptor_value_callbacks_.push_back(
      std::make_pair(callback, error_callback));
  task_manager_->PostWriteDescriptorValue(
      service_path_, descriptor_info_.get(), new_value,
      base::Bind(
          &BluetoothRemoteGattDescriptorWin::WriteRemoteDescriptorValueCallback,
          weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothRemoteGattDescriptorWin::Update() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  task_manager_->PostReadDescriptorValue(
      service_path_, descriptor_info_.get(),
      base::Bind(
          &BluetoothRemoteGattDescriptorWin::ReadRemoteDescriptorValueCallback,
          weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace device.