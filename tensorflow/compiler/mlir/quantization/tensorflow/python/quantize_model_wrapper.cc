/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "tensorflow/compiler/mlir/quantization/tensorflow/python/quantize_model_wrapper.h"

#include <optional>
#include <string>
#include <utility>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "pybind11/pybind11.h"
#include "tensorflow/compiler/mlir/quantization/tensorflow/calibrator/calibrator_singleton.h"
#include "tensorflow/compiler/mlir/quantization/tensorflow/python/quantize_model.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/python/lib/core/pybind11_lib.h"

namespace tensorflow {
namespace quantization {
namespace {

// Serializes GraphDef to python bytes object. Raises python ValueError if
// serialization fails.
std::string SerializeGraphDef(const GraphDef& graph_def,
                              const absl::string_view function_name,
                              const int line_no) {
  const std::string graph_def_serialized = graph_def.SerializeAsString();

  // Empty string means it failed to serialize the protobuf with an error. See
  // the docstring for SerializeAsString for details.
  if (graph_def_serialized.empty()) {
    throw py::value_error(absl::StrFormat(
        "Failed to serialize GraphDef result from function %s [%s:%d].",
        function_name, __FILE__, line_no));
  }

  return graph_def_serialized;
}

}  // namespace

std::string QuantizeQatModel(const absl::string_view saved_model_path,
                             const absl::string_view exported_names_str,
                             const absl::string_view tags,
                             const absl::string_view quant_opts_serialized) {
  const absl::StatusOr<GraphDef> graph_def = internal::QuantizeQatModel(
      saved_model_path, exported_names_str, tags, quant_opts_serialized);
  if (!graph_def.ok()) {
    throw py::value_error(absl::StrFormat("Failed to quantize QAT model: %s",
                                          graph_def.status().message()));
  }

  return SerializeGraphDef(*graph_def, __func__, __LINE__);
}

std::string QuantizePtqDynamicRange(
    const absl::string_view saved_model_path,
    const absl::string_view exported_names_str, const absl::string_view tags,
    const absl::string_view quant_opts_serialized) {
  const absl::StatusOr<GraphDef> graph_def = internal::QuantizePtqDynamicRange(
      saved_model_path, exported_names_str, tags, quant_opts_serialized);
  if (!graph_def.ok()) {
    throw py::value_error(
        absl::StrFormat("Failed to apply post-training dynamic range "
                        "quantization to the model: %s",
                        graph_def.status().message()));
  }

  return SerializeGraphDef(*graph_def, __func__, __LINE__);
}

std::string QuantizePtqModelPreCalibration(
    const absl::string_view saved_model_path,
    const absl::string_view exported_names_str, const absl::string_view tags) {
  const absl::StatusOr<GraphDef> graph_def =
      internal::QuantizePtqModelPreCalibration(saved_model_path,
                                               exported_names_str, tags);
  if (!graph_def.ok()) {
    throw py::value_error(absl::StrFormat(
        "Failed to quantize PTQ model at the precalibration stage: %s",
        graph_def.status().message()));
  }

  return SerializeGraphDef(*graph_def, __func__, __LINE__);
}

std::string QuantizePtqModelPostCalibration(
    const absl::string_view saved_model_path,
    const absl::string_view exported_names_str, const absl::string_view tags,
    const absl::string_view quant_opts_serialized) {
  const absl::StatusOr<GraphDef> graph_def =
      internal::QuantizePtqModelPostCalibration(
          saved_model_path, exported_names_str, tags, quant_opts_serialized);
  if (!graph_def.ok()) {
    throw py::value_error(absl::StrFormat(
        "Failed to quantize PTQ model at the postcalibration stage: %s",
        graph_def.status().message()));
  }

  return SerializeGraphDef(*graph_def, __func__, __LINE__);
}

void ClearCollectedInformationFromCalibrator() {
  calibrator::CalibratorSingleton::ClearCollectedInformation();
}

void ClearDataFromCalibrator(absl::string_view id) {
  calibrator::CalibratorSingleton::ClearData(id);
}

float GetMinFromCalibrator(absl::string_view id) {
  std::optional<std::pair<float, float>> min_max =
      calibrator::CalibratorSingleton::GetMinMax(id);
  if (!min_max.has_value()) {
    throw py::value_error(absl::StrFormat(
        "No calibrated data; cannot find min value for '%s'", id));
  }

  return min_max->first;
}

float GetMaxFromCalibrator(absl::string_view id) {
  std::optional<std::pair<float, float>> min_max =
      calibrator::CalibratorSingleton::GetMinMax(id);
  if (!min_max.has_value()) {
    throw py::value_error(absl::StrFormat(
        "No calibrated data; cannot find max value for '%s'", id));
  }

  return min_max->second;
}

}  // namespace quantization
}  // namespace tensorflow
