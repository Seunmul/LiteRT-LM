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

#include "runtime/executor/executor_settings_base.h"

#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "runtime/util/test_utils.h"  // NOLINT

namespace litert::lm {
namespace {

TEST(LlmExecutorConfigTest, Backend) {
  Backend backend;
  std::stringstream oss;
  backend = Backend::CPU_ARTISAN;
  oss << backend;
  EXPECT_EQ(oss.str(), "CPU_ARTISAN");

  backend = Backend::GPU_ARTISAN;
  oss.str("");
  oss << backend;
  EXPECT_EQ(oss.str(), "GPU_ARTISAN");

  backend = Backend::GPU;
  oss.str("");
  oss << backend;
  EXPECT_EQ(oss.str(), "GPU");

  backend = Backend::CPU;
  oss.str("");
  oss << backend;
  EXPECT_EQ(oss.str(), "CPU");

  backend = Backend::GOOGLE_TENSOR_ARTISAN;
  oss.str("");
  oss << backend;
  EXPECT_EQ(oss.str(), "GOOGLE_TENSOR_ARTISAN");

  backend = Backend::NPU;
  oss.str("");
  oss << backend;
  EXPECT_EQ(oss.str(), "NPU");
}

TEST(LlmExecutorConfigTest, StringToBackend) {
  auto backend = GetBackendFromString("cpu_artisan");
  EXPECT_EQ(*backend, Backend::CPU_ARTISAN);
  backend = GetBackendFromString("gpu_artisan");
  EXPECT_EQ(*backend, Backend::GPU_ARTISAN);
  backend = GetBackendFromString("gpu");
  EXPECT_EQ(*backend, Backend::GPU);
  backend = GetBackendFromString("cpu");
  EXPECT_EQ(*backend, Backend::CPU);
  backend = GetBackendFromString("google_tensor_artisan");
  EXPECT_EQ(*backend, Backend::GOOGLE_TENSOR_ARTISAN);
  backend = GetBackendFromString("npu");
  EXPECT_EQ(*backend, Backend::NPU);
}

TEST(LlmExecutorConfigTest, ActivatonDataType) {
  ActivationDataType act;
  std::stringstream oss;
  act = ActivationDataType::FLOAT32;
  oss << act;
  EXPECT_EQ(oss.str(), "FLOAT32");

  act = ActivationDataType::FLOAT16;
  oss.str("");
  oss << act;
  EXPECT_EQ(oss.str(), "FLOAT16");
}

TEST(LlmExecutorConfigTest, FakeWeightsMode) {
  FakeWeightsMode fake_weights_mode;
  std::stringstream oss;
  fake_weights_mode = FakeWeightsMode::FAKE_WEIGHTS_NONE;
  oss << fake_weights_mode;
  EXPECT_EQ(oss.str(), "FAKE_WEIGHTS_NONE");

  fake_weights_mode = FakeWeightsMode::FAKE_WEIGHTS_8BITS_ALL_LAYERS;
  oss.str("");
  oss << fake_weights_mode;
  EXPECT_EQ(oss.str(), "FAKE_WEIGHTS_8BITS_ALL_LAYERS");

  fake_weights_mode = FakeWeightsMode::FAKE_WEIGHTS_ATTN_8_FFN_4_EMB_4;
  oss.str("");
  oss << fake_weights_mode;
  EXPECT_EQ(oss.str(), "FAKE_WEIGHTS_ATTN_8_FFN_4_EMB_4");
}

TEST(LlmExecutorConfigTest, FileFormat) {
  std::stringstream oss;

  oss.str("");
  oss << FileFormat::TFLITE;
  EXPECT_EQ(oss.str(), "TFLITE");

  oss.str("");
  oss << FileFormat::TASK;
  EXPECT_EQ(oss.str(), "TASK");

  oss.str("");
  oss << FileFormat::LITERT_LM;
  EXPECT_EQ(oss.str(), "LITERT_LM");
}

TEST(LlmExecutorConfigTest, ModelAssets) {
  auto model_assets = ModelAssets::Create("/path/to/model1");
  ASSERT_OK(model_assets);
  std::stringstream oss;
  oss << *model_assets;
  const std::string expected_output = R"(model_path: /path/to/model1
fake_weights_mode: FAKE_WEIGHTS_NONE
)";
  EXPECT_EQ(oss.str(), expected_output);
}

}  // namespace
}  // namespace litert::lm
