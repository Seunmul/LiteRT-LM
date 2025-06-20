// Copyright 2025 The ODML Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "runtime/executor/llm_executor_settings.h"

#include <iostream>
#include <memory>
#include <ostream>
#include <utility>
#include <variant>

#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "runtime/executor/executor_settings_base.h"
#include "runtime/util/logging.h"
#include "runtime/util/status_macros.h"  // NOLINT

namespace litert::lm {

std::ostream& operator<<(std::ostream& os, const GpuArtisanConfig& config) {
  os << "num_output_candidates: " << config.num_output_candidates << "\n";
  os << "wait_for_weight_uploads: " << config.wait_for_weight_uploads << "\n";
  os << "num_decode_steps_per_sync: " << config.num_decode_steps_per_sync
     << "\n";
  os << "sequence_batch_size: " << config.sequence_batch_size << "\n";
  os << "supported_lora_ranks: " << config.supported_lora_ranks << "\n";
  os << "max_top_k: " << config.max_top_k << "\n";
  os << "enable_decode_logits: " << config.enable_decode_logits << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const GpuConfig& config) {
  os << "max_top_k: " << config.max_top_k << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const CpuConfig& config) {
  os << "number_of_threads: " << config.number_of_threads << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const LlmExecutorSettings& config) {
  os << "backend: " << config.GetBackend() << "\n";
  std::visit(
      [&os](const auto& backend_config) {
        os << "backend_config: " << backend_config << "\n";
      },
      config.backend_config_);
  os << "max_tokens: " << config.GetMaxNumTokens() << "\n";
  os << "activation_data_type: " << config.GetActivationDataType() << "\n";
  os << "max_num_images: " << config.GetMaxNumImages() << "\n";
  os << "cache_dir: " << config.GetCacheDir() << "\n";
  if (config.GetScopedCacheFile()) {
    os << "cache_file: " << config.GetScopedCacheFile()->file() << "\n";
  } else {
    os << "cache_file: Not set.\n";
  }
  os << "model_assets: " << config.GetModelAssets() << "\n";
  return os;
}

// static
absl::StatusOr<LlmExecutorSettings> LlmExecutorSettings::CreateDefault(
    ModelAssets model_assets, Backend backend) {
  LlmExecutorSettings settings(std::move(model_assets));
  if (backend == Backend::CPU) {
    CpuConfig config;
    config.number_of_threads = 4;
    settings.SetBackendConfig(config);
  } else if (backend == Backend::GPU) {
    GpuConfig config;
    // Default max top k to 1 for GPU.
    config.max_top_k = 1;
    settings.SetBackendConfig(config);
  } else if (backend == Backend::NPU) {
  } else if (backend == Backend::GPU_ARTISAN) {
    settings.SetBackendConfig(GpuArtisanConfig());
  } else {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported backend: ", backend));
  }
  settings.SetBackend(backend);
  // Explicitly set the field value to avoid undefined behavior. Setting to 0
  // means that the maximum number of tokens is not set can could be inferred
  // from the model assets (but note that for the model or backend which does
  // not support this, an error will be thrown during initialization).
  settings.SetMaxNumTokens(0);
  // Disable image input by default.
  settings.SetMaxNumImages(0);
  return std::move(settings);
}

}  // namespace litert::lm
