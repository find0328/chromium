// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_remote_gatt_characteristic_win.h"

#include "base/bind.h"
#include "base/thread_task_runner_handle.h"
#include "device/bluetooth/bluetooth_gatt_notify_session_win.h"

namespace device {

void WinOnRemoteCharacteristicValueChanged(BTH_LE_GATT_EVENT_TYPE type,
                                           PVOID event_parameter,
                                           PVOID context) {
  BluetoothRemoteGattCharacteristicWin* characteristic =
      (BluetoothRemoteGattCharacteristicWin*)context;
  characteristic->OnRemoteCharacteristicValueChanged(type, event_parameter);
}

void BluetoothRemoteGattCharacteristicWin::
    NotifyCharacteristicDiscComplIfNecessary() {
  if (descriptor_discovered_ &&
      completed_descriptors_.size() == included_descriptor_objects_.size() &&
      (characteristic_value_initialized_ ||
       !characteristic_info_->IsReadable) &&
      !complete_notified_) {
    parent_service_->NotifyGattCharacteristicAdded(this);
    complete_notified_ = true;
  }
}

void BluetoothRemoteGattCharacteristicWin::UpdateIncludedDescriptors(
    PBTH_LE_GATT_DESCRIPTOR descriptors,
    uint16_t num) {
  if (num == 0) {
    completed_descriptors_.clear();
    included_descriptor_objects_.clear();
    return;
  }
  // Map of retrieved descriptor uuid value to its index in |descriptors|.
  std::map<std::string, uint16_t> current_descriptors;
  for (uint16_t i = 0; i < num; i++) {
    current_descriptors[task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
                                         descriptors[i].DescriptorUuid)
                            .value()] = i;
  }
  // Map of known descriptor uuid value to its identifier.
  std::map<std::string, std::string> known_descriptors;
  if (included_descriptor_objects_.size() != 0) {
    for (auto e : included_descriptor_objects_) {
      known_descriptors[e.second->GetUUID().value()] = e.first;
    }
    std::vector<std::string> removed_descriptors;
    for (auto e : known_descriptors) {
      if (current_descriptors.find(e.first) == current_descriptors.end()) {
        removed_descriptors.push_back(e.second);
      }
    }
    for (auto e : removed_descriptors) {
      completed_descriptors_.erase(e);
      included_descriptor_objects_.erase(e);
    }
    // Update previously known descriptors.
    for (auto e : included_descriptor_objects_)
      e.second->Update();
  }

  // Return if no new descriptors have been added.
  if (included_descriptor_objects_.size() == num)
    return;

  // Add new descriptors.
  for (auto e : current_descriptors) {
    if (known_descriptors.find(e.first) == known_descriptors.end()) {
      PBTH_LE_GATT_DESCRIPTOR win_descriptor_info =
          new BTH_LE_GATT_DESCRIPTOR();
      *win_descriptor_info = descriptors[e.second];
      BluetoothRemoteGattDescriptorWin* descriptor_object =
          new BluetoothRemoteGattDescriptorWin(
              adapter_, parent_service_->GetServicePath(), this,
              win_descriptor_info, ui_task_runner_);
      included_descriptor_objects_.add(
          descriptor_object->GetIdentifier(),
          scoped_ptr<BluetoothRemoteGattDescriptorWin>(descriptor_object));
    }
  }
}

void BluetoothRemoteGattCharacteristicWin::GetIncludedDescriptorsCallback(
    PBTH_LE_GATT_DESCRIPTOR descriptors,
    uint16_t num,
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    return;
  descriptor_discovered_ = true;
  UpdateIncludedDescriptors(descriptors, num);
  NotifyCharacteristicDiscComplIfNecessary();
}

// Called by included descriptors.
void BluetoothRemoteGattCharacteristicWin::NotifyGattDescriptorAdded(
    BluetoothRemoteGattDescriptorWin* descriptor) {
  completed_descriptors_.insert(descriptor->GetIdentifier());
  adapter_->NotifyGattDescriptorAdded(descriptor);
  NotifyCharacteristicDiscComplIfNecessary();
}

void BluetoothRemoteGattCharacteristicWin::NotifyGattCharacteristicValueChanged(
    uint8_t* new_value,
    ULONG size) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  characteristic_value_.clear();
  for (ULONG i = 0; i < size; i++)
    characteristic_value_.push_back(new_value[i]);
  adapter_->NotifyGattCharacteristicValueChanged(this, characteristic_value_);
}

void BluetoothRemoteGattCharacteristicWin::OnRemoteCharacteristicValueChanged(
    BTH_LE_GATT_EVENT_TYPE type,
    PVOID event_parameter) {
  if (type == CharacteristicValueChangedEvent) {
    BLUETOOTH_GATT_VALUE_CHANGED_EVENT* event =
        (BLUETOOTH_GATT_VALUE_CHANGED_EVENT*)event_parameter;
    PBTH_LE_GATT_CHARACTERISTIC_VALUE new_value_win =
        event->CharacteristicValue;

    uint8_t* new_value = new uint8_t[new_value_win->DataSize];
    memcpy(new_value, &(new_value_win->Data[0]), new_value_win->DataSize);
    ui_task_runner_->PostTask(
        FROM_HERE, base::Bind(&BluetoothRemoteGattCharacteristicWin::
                                  NotifyGattCharacteristicValueChanged,
                              weak_ptr_factory_.GetWeakPtr(),
                              base::Owned(new_value), new_value_win->DataSize));
  }
}

void BluetoothRemoteGattCharacteristicWin::GattEventRegistrationCallback(
    BLUETOOTH_GATT_EVENT_HANDLE event_handle,
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr)) {
    for (auto callback : start_notifying_callback_)
      callback.second.Run(BluetoothGattService::GATT_ERROR_FAILED);
    start_notifying_callback_.clear();
    return;
  }

  for (auto callback : start_notifying_callback_) {
    scoped_ptr<BluetoothGattNotifySessionWin> notify_session(
        new BluetoothGattNotifySessionWin(weak_ptr_factory_.GetWeakPtr()));
    callback.first.Run(std::move(notify_session));
    number_of_active_notify_sessions_++;
  }
  start_notifying_callback_.clear();
  registered_event_handle_ = event_handle;
  is_notifying_ = true;
}

BluetoothRemoteGattCharacteristicWin::BluetoothRemoteGattCharacteristicWin(
    BluetoothRemoteGattServiceWin* parent_service,
    BTH_LE_GATT_CHARACTERISTIC* characteristic_info,
    scoped_refptr<base::SequencedTaskRunner>& ui_task_runner)
    : parent_service_(parent_service),
      characteristic_info_(characteristic_info),
      ui_task_runner_(ui_task_runner),
      characteristic_value_initialized_(false),
      registered_event_handle_(NULL),
      weak_ptr_factory_(this),
      complete_notified_(false),
      is_notifying_(false),
      number_of_active_notify_sessions_(0) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  characteristic_value_.clear();
  included_descriptor_objects_.clear();
  completed_descriptors_.clear();
  read_remote_characteristic_value_callbacks_.clear();
  write_remote_characteristic_value_callback_.clear();
  start_notifying_callback_.clear();

  adapter_ = parent_service_->GetAdapter();
  task_manager_ = adapter_->GetWinBluetoothTaskManager();
  DCHECK(parent_service_);
  DCHECK(characteristic_info_);
  DCHECK(adapter_);
  DCHECK(task_manager_);

  characteristic_uuid_ = task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
      characteristic_info_->CharacteristicUuid);
  Update();
}

BluetoothRemoteGattCharacteristicWin::~BluetoothRemoteGattCharacteristicWin() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (registered_event_handle_ != NULL) {
    task_manager_->UnregisterCharacteristicValueChangedEvent(
        registered_event_handle_);
  }

  completed_descriptors_.clear();
  included_descriptor_objects_.clear();
  adapter_->NotifyGattCharacteristicRemoved(this);
}

std::string BluetoothRemoteGattCharacteristicWin::GetIdentifier() const {
  return parent_service_->GetIdentifier() + "_" + characteristic_uuid_.value();
}

BluetoothUUID BluetoothRemoteGattCharacteristicWin::GetUUID() const {
  return characteristic_uuid_;
}

bool BluetoothRemoteGattCharacteristicWin::IsLocal() const {
  return false;
}

std::vector<uint8_t>& BluetoothRemoteGattCharacteristicWin::GetValue() const {
  return const_cast<std::vector<uint8_t>&>(characteristic_value_);
}

BluetoothGattService* BluetoothRemoteGattCharacteristicWin::GetService() const {
  return parent_service_;
}

BluetoothGattCharacteristic::Properties
BluetoothRemoteGattCharacteristicWin::GetProperties() const {
  BluetoothGattCharacteristic::Properties properties = PROPERTY_NONE;

  if (characteristic_info_.get()->IsBroadcastable)
    properties = properties | PROPERTY_BROADCAST;
  if (characteristic_info_.get()->IsReadable)
    properties = properties | PROPERTY_READ;
  if (characteristic_info_.get()->IsWritableWithoutResponse)
    properties = properties | PROPERTY_WRITE_WITHOUT_RESPONSE;
  if (characteristic_info_.get()->IsWritable)
    properties = properties | PROPERTY_WRITE;
  if (characteristic_info_.get()->IsNotifiable)
    properties = properties | PROPERTY_NOTIFY;
  if (characteristic_info_.get()->IsIndicatable)
    properties = properties | PROPERTY_INDICATE;
  if (characteristic_info_.get()->IsSignedWritable)
    properties = properties | PROPERTY_AUTHENTICATED_SIGNED_WRITES;
  if (characteristic_info_.get()->HasExtendedProperties)
    properties = properties | PROPERTY_EXTENDED_PROPERTIES;

  return properties;
}

BluetoothGattCharacteristic::Permissions
BluetoothRemoteGattCharacteristicWin::GetPermissions() const {
  BluetoothGattCharacteristic::Permissions permissions = PERMISSION_NONE;

  if (characteristic_info_.get()->IsReadable)
    permissions = permissions | PERMISSION_READ;
  if (characteristic_info_.get()->IsWritable)
    permissions = permissions | PERMISSION_WRITE;

  return permissions;
}

bool BluetoothRemoteGattCharacteristicWin::IsNotifying() const {
  return is_notifying_;
}

std::vector<BluetoothGattDescriptor*>
BluetoothRemoteGattCharacteristicWin::GetDescriptors() const {
  std::vector<BluetoothGattDescriptor*> descriptors;
  for (auto descriptor : included_descriptor_objects_)
    descriptors.push_back(descriptor.second);
  return descriptors;
}

BluetoothGattDescriptor* BluetoothRemoteGattCharacteristicWin::GetDescriptor(
    const std::string& identifier) const {
  GattDescriptorMap::const_iterator it =
      included_descriptor_objects_.find(identifier);
  if (it != included_descriptor_objects_.end())
    return it->second;
  return nullptr;
}

bool BluetoothRemoteGattCharacteristicWin::AddDescriptor(
    BluetoothGattDescriptor* descriptor) {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothRemoteGattCharacteristicWin::UpdateValue(
    const std::vector<uint8_t>& value) {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothRemoteGattCharacteristicWin::StartNotifySession(
    const NotifySessionCallback& callback,
    const ErrorCallback& error_callback) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (is_notifying_) {
    scoped_ptr<BluetoothGattNotifySessionWin> notify_session(
        new BluetoothGattNotifySessionWin(weak_ptr_factory_.GetWeakPtr()));
    callback.Run(std::move(notify_session));
    number_of_active_notify_sessions_++;
    return;
  }

  if (characteristic_info_->IsNotifiable ||
      characteristic_info_->IsIndicatable) {
    start_notifying_callback_.push_back(
        std::make_pair(callback, error_callback));
    task_manager_->PostRegisterCharacteristicValueChangedEvent(
        parent_service_->GetServicePath(), characteristic_info_.get(),
        base::Bind(&BluetoothRemoteGattCharacteristicWin::
                       GattEventRegistrationCallback,
                   weak_ptr_factory_.GetWeakPtr()),
        &WinOnRemoteCharacteristicValueChanged, (PVOID) this);
  } else {
    error_callback.Run(BluetoothGattService::GATT_ERROR_NOT_SUPPORTED);
  }
}

void BluetoothRemoteGattCharacteristicWin::StopNotifySession() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (!is_notifying_)
    return;

  number_of_active_notify_sessions_--;
  if (number_of_active_notify_sessions_ > 0)
    return;
  task_manager_->UnregisterCharacteristicValueChangedEvent(
      registered_event_handle_);
  registered_event_handle_ = NULL;
  is_notifying_ = false;
  number_of_active_notify_sessions_ = 0;
  start_notifying_callback_.clear();
}

void BluetoothRemoteGattCharacteristicWin::ReadCharacteristicValueCallback(
    PBTH_LE_GATT_CHARACTERISTIC_VALUE value,
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed at reading characteristic value with error ";
    for (const auto callback : read_remote_characteristic_value_callbacks_)
      callback.second.Run(BluetoothGattService::GATT_ERROR_FAILED);
  } else {
    characteristic_value_.clear();
    for (ULONG i = 0; i < value->DataSize; i++)
      characteristic_value_.push_back(value->Data[i]);
    for (const auto callback : read_remote_characteristic_value_callbacks_)
      callback.first.Run(characteristic_value_);
  }

  if (!characteristic_value_initialized_) {
    characteristic_value_initialized_ = true;
    NotifyCharacteristicDiscComplIfNecessary();
  }
  read_remote_characteristic_value_callbacks_.clear();
}

void BluetoothRemoteGattCharacteristicWin::ReadRemoteCharacteristic(
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (!characteristic_info_.get()->IsReadable)
    error_callback.Run(BluetoothGattService::GATT_ERROR_NOT_PERMITTED);

  read_remote_characteristic_value_callbacks_.push_back(
      std::make_pair(callback, error_callback));
  task_manager_->PostReadCharacteristicValue(
      parent_service_->GetServicePath(), characteristic_info_.get(),
      base::Bind(&BluetoothRemoteGattCharacteristicWin::
                     ReadCharacteristicValueCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothRemoteGattCharacteristicWin::WriteRemoteCharacteristicCallback(
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr)) {
    for (const auto callback : write_remote_characteristic_value_callback_)
      callback.second.Run(BluetoothGattService::GATT_ERROR_FAILED);
  } else {
    for (const auto callback : write_remote_characteristic_value_callback_)
      callback.first.Run();
  }
  write_remote_characteristic_value_callback_.clear();
}

void BluetoothRemoteGattCharacteristicWin::WriteRemoteCharacteristic(
    const std::vector<uint8_t>& new_value,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (!characteristic_info_.get()->IsWritable)
    error_callback.Run(BluetoothGattService::GATT_ERROR_NOT_PERMITTED);

  write_remote_characteristic_value_callback_.push_back(
      std::make_pair(callback, error_callback));
  task_manager_->PostWriteCharacteristicValue(
      parent_service_->GetServicePath(), characteristic_info_.get(), new_value,
      base::Bind(&BluetoothRemoteGattCharacteristicWin::
                     WriteRemoteCharacteristicCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothRemoteGattCharacteristicWin::Update() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  task_manager_->PostGetGattIncludedDescriptors(
      parent_service_->GetServicePath(), characteristic_info_.get(),
      base::Bind(
          &BluetoothRemoteGattCharacteristicWin::GetIncludedDescriptorsCallback,
          weak_ptr_factory_.GetWeakPtr()));

  if (characteristic_info_->IsReadable) {
    task_manager_->PostReadCharacteristicValue(
        parent_service_->GetServicePath(), characteristic_info_.get(),
        base::Bind(&BluetoothRemoteGattCharacteristicWin::
                       ReadCharacteristicValueCallback,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

}  // namespace device.
