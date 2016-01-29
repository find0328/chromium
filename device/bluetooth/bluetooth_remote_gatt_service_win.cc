// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_remote_gatt_service_win.h"

#include "base/bind.h"

namespace device {

void BluetoothRemoteGattServiceWin::NotifyServiceDiscComplIfNecessary() {
  if (discovery_completed_included_charateristics_.size() ==
          included_characteristic_objects_.size() &&
      discovery_completed_included_services_.size() ==
          included_service_objects_.size() &&
      included_services_discovered_ && included_characteristics_discovered_ &&
      !complete_notified_) {
    if (is_primary_)
      adapter_->NotifyGattDiscoveryCompleteForService(this);
    else
      parent_service_->NotifyGattServiceAdded(this);
    complete_notified_ = true;
  }
}

void BluetoothRemoteGattServiceWin::UpdateIncludedCharacteristics(
    PBTH_LE_GATT_CHARACTERISTIC characteristics,
    uint16_t num) {
  if (num == 0) {
    discovery_completed_included_charateristics_.clear();
    included_characteristic_objects_.clear();
    return;
  }

  // Map of retreived characteristic uuid value to its index in
  // |characteristics|.
  std::map<std::string, uint16_t> current_characteristics;
  for (uint16_t i = 0; i < num; i++) {
    current_characteristics[task_manager_
                                ->BluetoothLowEnergyUuidToBluetoothUuid(
                                    characteristics[i].CharacteristicUuid)
                                .value()] = i;
  }

  // Map of known characteristics uuid value to its identifier.
  std::map<std::string, std::string> known_characteristics;
  // Remove no longer existed characteristics first.
  if (included_characteristic_objects_.size() != 0) {
    for (auto e : included_characteristic_objects_) {
      known_characteristics[e.second->GetUUID().value()] = e.first;
    }
    std::vector<std::string> removed_characteristics;
    for (auto e : known_characteristics) {
      if (current_characteristics.find(e.first) ==
          current_characteristics.end()) {
        removed_characteristics.push_back(e.second);
      }
    }
    for (auto identifier : removed_characteristics) {
      discovery_completed_included_charateristics_.erase(identifier);
      included_characteristic_objects_.erase(identifier);
    }
    // Update previously known characteristics.
    for (auto e : included_characteristic_objects_)
      e.second->Update();
  }

  // Return if no new characteristics have been added.
  if (included_characteristic_objects_.size() == num)
    return;

  // Add new characteristics.
  for (auto e : current_characteristics) {
    if (known_characteristics.find(e.first) == known_characteristics.end()) {
      PBTH_LE_GATT_CHARACTERISTIC info = new BTH_LE_GATT_CHARACTERISTIC();
      *info = characteristics[e.second];
      BluetoothRemoteGattCharacteristicWin* characteristic_object =
          new BluetoothRemoteGattCharacteristicWin(this, info, ui_task_runner_);
      included_characteristic_objects_.add(
          characteristic_object->GetIdentifier(),
          scoped_ptr<BluetoothRemoteGattCharacteristicWin>(
              characteristic_object));
    }
  }
}

void BluetoothRemoteGattServiceWin::GetIncludedCharacteristicsCallback(
    PBTH_LE_GATT_CHARACTERISTIC characteristics,
    uint16_t num,
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    return;
  UpdateIncludedCharacteristics(characteristics, num);
  included_characteristics_discovered_ = true;
  NotifyServiceDiscComplIfNecessary();
}

void BluetoothRemoteGattServiceWin::UpdateIncludedServices(
    PBTH_LE_GATT_SERVICE services,
    uint16_t num) {
  if (num == 0) {
    discovery_completed_included_services_.clear();
    included_service_objects_.clear();
    return;
  }

  // Map of retreived service uuid value to its index in
  // |services|.
  std::map<std::string, uint16_t> current_services;
  for (uint16_t i = 0; i < num; i++) {
    current_services[task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
                                      services[i].ServiceUuid)
                         .value()] = i;
  }

  // Map of known services uuid value to its identifier.
  std::map<std::string, std::string> known_services;
  // Remove no longer existed services first.
  if (included_service_objects_.size() != 0) {
    for (auto e : included_service_objects_) {
      known_services[e.second->GetUUID().value()] = e.first;
    }
    std::vector<std::string> removed_services;
    for (auto e : known_services) {
      if (current_services.find(e.first) == current_services.end()) {
        removed_services.push_back(e.second);
      }
    }
    for (auto identifier : removed_services) {
      discovery_completed_included_services_.erase(identifier);
      included_service_objects_.erase(identifier);
    }
    // Update previously known included services.
    for (auto e : included_service_objects_)
      e.second->Update();
  }

  // Return if no new services have been added.
  if (included_service_objects_.size() == num)
    return;

  // Add new services.
  for (auto e : current_services) {
    if (known_services.find(e.first) == known_services.end()) {
      BluetoothUUID uuid = task_manager_->BluetoothLowEnergyUuidToBluetoothUuid(
          services[e.second].ServiceUuid);
      BluetoothRemoteGattServiceWin* service_object =
          new BluetoothRemoteGattServiceWin(device_, service_path_, uuid,
                                            services[e.second].AttributeHandle,
                                            false, this, ui_task_runner_);
      included_service_objects_.add(
          service_object->GetIdentifier(),
          scoped_ptr<BluetoothRemoteGattServiceWin>(service_object));
    }
  }
}

void BluetoothRemoteGattServiceWin::GetIncludedServicesCallback(
    PBTH_LE_GATT_SERVICE services,
    uint16_t num,
    HRESULT hr) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    return;
  UpdateIncludedServices(services, num);
  included_services_discovered_ = true;
  NotifyServiceDiscComplIfNecessary();
}

// Called by included service.
void BluetoothRemoteGattServiceWin::NotifyGattServiceAdded(
    BluetoothRemoteGattServiceWin* service) {
  discovery_completed_included_services_.insert(service->GetIdentifier());
  NotifyServiceDiscComplIfNecessary();
}

// Called by included characteristics.
void BluetoothRemoteGattServiceWin::NotifyGattCharacteristicAdded(
    BluetoothRemoteGattCharacteristicWin* characteristic) {
  discovery_completed_included_charateristics_.insert(
      characteristic->GetIdentifier());
  adapter_->NotifyGattCharacteristicAdded(characteristic);
  NotifyServiceDiscComplIfNecessary();
}

BluetoothRemoteGattServiceWin::BluetoothRemoteGattServiceWin(
    BluetoothDeviceWin* device,
    base::FilePath service_path,
    BluetoothUUID service_uuid,
    uint16_t service_attribute_handle,
    bool is_primary,
    BluetoothRemoteGattServiceWin* parent_service,
    scoped_refptr<base::SequencedTaskRunner>& ui_task_runner)
    : service_path_(service_path),
      device_(device),
      service_uuid_(service_uuid),
      service_attribute_handle_(service_attribute_handle),
      is_primary_(is_primary),
      parent_service_(parent_service),
      adapter_(nullptr),
      task_manager_(nullptr),
      complete_notified_(false),
      included_services_discovered_(false),
      included_characteristics_discovered_(false),
      ui_task_runner_(ui_task_runner),
      weak_ptr_factory_(this) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  discovery_completed_included_charateristics_.clear();
  discovery_completed_included_services_.clear();
  included_characteristic_objects_.clear();
  included_service_objects_.clear();
  adapter_ = const_cast<BluetoothAdapterWin*>(device_->GetAdapter());
  task_manager_ = adapter_->GetWinBluetoothTaskManager();
  DCHECK(device_);
  if (!is_primary_)
    DCHECK(parent_service_);
  DCHECK(adapter_);
  DCHECK(task_manager_);
  Update();
}

BluetoothRemoteGattServiceWin::~BluetoothRemoteGattServiceWin() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  included_service_objects_.clear();
  included_characteristic_objects_.clear();
  discovery_completed_included_charateristics_.clear();
  discovery_completed_included_services_.clear();
  included_services_discovered_ = false;
  included_characteristics_discovered_ = false;

  // Only primary service notify adapter the service removed. The life cycle of
  // included service is controlled by its parent service.
  if (is_primary_)
    adapter_->NotifyGattServiceRemoved(device_, this);
}

std::string BluetoothRemoteGattServiceWin::GetIdentifier() const {
  std::string identifier =
      service_uuid_.value() + "_" + std::to_string(service_attribute_handle_);
  if (is_primary_)
    return device_->GetIdentifier() + "_" + identifier;
  else
    return parent_service_->GetIdentifier() + "_" + identifier;
}

BluetoothUUID BluetoothRemoteGattServiceWin::GetUUID() const {
  return const_cast<BluetoothUUID&>(service_uuid_);
}

bool BluetoothRemoteGattServiceWin::IsLocal() const {
  return false;
}

bool BluetoothRemoteGattServiceWin::IsPrimary() const {
  return is_primary_;
}

BluetoothDevice* BluetoothRemoteGattServiceWin::GetDevice() const {
  return device_;
}

std::vector<BluetoothGattCharacteristic*>
BluetoothRemoteGattServiceWin::GetCharacteristics() const {
  std::vector<BluetoothGattCharacteristic*> has_characteristics;
  for (auto c : included_characteristic_objects_)
    has_characteristics.push_back(c.second);
  return has_characteristics;
}

std::vector<BluetoothGattService*>
BluetoothRemoteGattServiceWin::GetIncludedServices() const {
  std::vector<BluetoothGattService*> has_services;
  for (auto s : included_service_objects_)
    has_services.push_back(s.second);
  return has_services;
}

BluetoothGattCharacteristic* BluetoothRemoteGattServiceWin::GetCharacteristic(
    const std::string& identifier) const {
  GattCharacteristicsMap::const_iterator it =
      included_characteristic_objects_.find(identifier);
  if (it != included_characteristic_objects_.end())
    return it->second;
  return nullptr;
}

bool BluetoothRemoteGattServiceWin::AddCharacteristic(
    device::BluetoothGattCharacteristic* characteristic) {
  NOTIMPLEMENTED();
  return false;
}

bool BluetoothRemoteGattServiceWin::AddIncludedService(
    device::BluetoothGattService* service) {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothRemoteGattServiceWin::Register(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run();
}

void BluetoothRemoteGattServiceWin::Unregister(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run();
}

void BluetoothRemoteGattServiceWin::Update() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  task_manager_->PostGetGattIncludedServices(
      service_path_, service_uuid_, service_attribute_handle_,
      base::Bind(&BluetoothRemoteGattServiceWin::GetIncludedServicesCallback,
                 weak_ptr_factory_.GetWeakPtr()));
  task_manager_->PostGetGattIncludedCharacteristics(
      service_path_, service_uuid_, service_attribute_handle_,
      base::Bind(
          &BluetoothRemoteGattServiceWin::GetIncludedCharacteristicsCallback,
          weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace device.
