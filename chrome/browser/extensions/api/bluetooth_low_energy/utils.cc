// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/bluetooth_low_energy/utils.h"

namespace extensions {
namespace api {
namespace bluetooth_low_energy {

namespace {

// Converts a list of CharacteristicProperty to a base::ListValue of strings.
scoped_ptr<base::ListValue> CharacteristicPropertiesToValue(
    const std::vector<CharacteristicProperty> properties) {
  scoped_ptr<base::ListValue> property_list(new base::ListValue());
  for (std::vector<CharacteristicProperty>::const_iterator iter =
           properties.begin();
       iter != properties.end();
       ++iter)
    property_list->Append(new base::StringValue(ToString(*iter)));
  return property_list.Pass();
}

}  // namespace

scoped_ptr<base::DictionaryValue> CharacteristicToValue(Characteristic* from) {
  // Copy the properties. Use Characteristic::ToValue to generate the result
  // dictionary without the properties, to prevent json_schema_compiler from
  // failing.
  std::vector<CharacteristicProperty> properties = from->properties;
  from->properties.clear();
  scoped_ptr<base::DictionaryValue> to = from->ToValue();
  to->SetWithoutPathExpansion(
      "properties", CharacteristicPropertiesToValue(properties).release());
  return to.Pass();
}

scoped_ptr<base::DictionaryValue> DescriptorToValue(Descriptor* from) {
  // Copy the characteristic properties and set them later manually.
  std::vector<CharacteristicProperty> properties =
      from->characteristic.properties;
  from->characteristic.properties.clear();
  scoped_ptr<base::DictionaryValue> to = from->ToValue();

  base::DictionaryValue* chrc_value = NULL;
  to->GetDictionaryWithoutPathExpansion("characteristic", &chrc_value);
  DCHECK(chrc_value);
  chrc_value->SetWithoutPathExpansion(
      "properties", CharacteristicPropertiesToValue(properties).release());
  return to.Pass();
}

}  // namespace bluetooth_low_energy
}  // namespace api
}  // namespace extensions
