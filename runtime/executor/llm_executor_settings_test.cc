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

#include <filesystem>  // NOLINT: Required for path manipulation.
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl
#include "runtime/util/scoped_file.h"
#include "runtime/util/test_utils.h"  // NOLINT

namespace litert::lm {
namespace {

using absl::StatusCode::kInvalidArgument;
using ::testing::VariantWith;
using ::testing::status::StatusIs;

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

GpuArtisanConfig CreateGpuArtisanConfig() {
  GpuArtisanConfig config;
  config.num_output_candidates = 1;
  config.wait_for_weight_uploads = true;
  config.num_decode_steps_per_sync = 3;
  config.sequence_batch_size = 16;
  config.supported_lora_ranks = {4, 16};
  config.max_top_k = 40;
  config.enable_decode_logits = true;
  return config;
}

TEST(LlmExecutorConfigTest, GpuArtisanConfig) {
  GpuArtisanConfig config = CreateGpuArtisanConfig();
  std::stringstream oss;
  oss << config;
  const std::string expected_output = R"(num_output_candidates: 1
wait_for_weight_uploads: 1
num_decode_steps_per_sync: 3
sequence_batch_size: 16
supported_lora_ranks: vector of 2 elements: [4, 16]
max_top_k: 40
enable_decode_logits: 1
)";
  EXPECT_EQ(oss.str(), expected_output);
}

TEST(LlmExecutorConfigTest, LlmExecutorSettings) {
  auto model_assets = ModelAssets::Create("/path/to/model1");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets, Backend::GPU_ARTISAN);
  (*settings).SetBackendConfig(CreateGpuArtisanConfig());
  (*settings).SetMaxNumTokens(1024);
  (*settings).SetActivationDataType(ActivationDataType::FLOAT16);
  (*settings).SetMaxNumImages(1);
  (*settings).SetCacheDir("/path/to/cache");

  std::stringstream oss;
  oss << (*settings);
  const std::string expected_output = R"(backend: GPU_ARTISAN
backend_config: num_output_candidates: 1
wait_for_weight_uploads: 1
num_decode_steps_per_sync: 3
sequence_batch_size: 16
supported_lora_ranks: vector of 2 elements: [4, 16]
max_top_k: 40
enable_decode_logits: 1

max_tokens: 1024
activation_data_type: FLOAT16
max_num_images: 1
cache_dir: /path/to/cache
cache_file: Not set.
model_assets: model_path: /path/to/model1
fake_weights_mode: FAKE_WEIGHTS_NONE

)";
  EXPECT_EQ(oss.str(), expected_output);
}

TEST(GetWeightCacheFileTest, CacheDirAndModelPath) {
  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetCacheDir("/weight/cache/path");

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file, settings->GetWeightCacheFile());
  EXPECT_THAT(weight_cache_file, VariantWith<std::string>(
                                     "/weight/cache/path/model1.tflite.cache"));
}

TEST(GetWeightCacheFileTest, CacheDirHasTrailingSeparator) {
  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetCacheDir("/weight/cache/path/");

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file, settings->GetWeightCacheFile());
  EXPECT_THAT(weight_cache_file, VariantWith<std::string>(
                              "/weight/cache/path/model1.tflite.cache"));
}

TEST(GetWeightCacheFileTest, CacheDirAndModelPathAndCustomSuffix) {
  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetCacheDir("/weight/cache/path");

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file,
                       settings->GetWeightCacheFile(".xnnpack_cache"));
  EXPECT_THAT(weight_cache_file,
              VariantWith<std::string>(
                  "/weight/cache/path/model1.tflite.xnnpack_cache"));
}

TEST(LlmExecutorConfigTest, ModelPathOnly) {
  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file, settings->GetWeightCacheFile());
  EXPECT_THAT(weight_cache_file,
              VariantWith<std::string>("/path/to/model1.tflite.cache"));
}

TEST(GetWeightCacheFileTest, ModelPathAndSuffix) {
  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file,
                       settings->GetWeightCacheFile(".custom_suffix"));
  EXPECT_THAT(weight_cache_file,
              VariantWith<std::string>("/path/to/model1.tflite.custom_suffix"));
}

TEST(GetWeightCacheFileTest, PreferScopedCacheFileToCacheDir) {
  const std::string cache_path =
      std::filesystem::path(::testing::SrcDir()) /
      "litert_lm/runtime/testdata/test_lm.cache";

  ASSERT_OK_AND_ASSIGN(auto cache_file, ScopedFile::Open(cache_path));
  auto shared_cache_file = std::make_shared<ScopedFile>(std::move(cache_file));

  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetScopedCacheFile(shared_cache_file);
  settings->SetCacheDir("/weight/cache/path");

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file, settings->GetWeightCacheFile());
  EXPECT_THAT(weight_cache_file,
              VariantWith<std::shared_ptr<ScopedFile>>(shared_cache_file));
}

TEST(GetWeightCacheFileTest, PreferScopedCacheFileToScopedModelFile) {
  const std::string model_path =
      std::filesystem::path(::testing::SrcDir()) /
      "litert_lm/runtime/testdata/test_lm.litertlm";
  const std::string cache_path =
      std::filesystem::path(::testing::SrcDir()) /
      "litert_lm/runtime/testdata/test_lm.cache";

  ASSERT_OK_AND_ASSIGN(auto model_file, ScopedFile::Open(model_path));
  ASSERT_OK_AND_ASSIGN(auto cache_file, ScopedFile::Open(cache_path));
  auto shared_cache_file = std::make_shared<ScopedFile>(std::move(cache_file));

  auto model_assets =
      ModelAssets::Create(std::make_shared<ScopedFile>(std::move(model_file)));
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetScopedCacheFile(shared_cache_file);

  ASSERT_OK_AND_ASSIGN(auto weight_cache_file, settings->GetWeightCacheFile());
  EXPECT_THAT(weight_cache_file,
              VariantWith<std::shared_ptr<ScopedFile>>(shared_cache_file));
}

TEST(GetWeightCacheFileTest, EmptyModelPath) {
  auto model_assets = ModelAssets::Create("");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetCacheDir("/weight/cache/path");

  EXPECT_THAT(settings->GetWeightCacheFile(".xnnpack_cache"),
              StatusIs(kInvalidArgument));
}

TEST(GetWeightCacheFileTest, CacheDisabled) {
  const std::string cache_path =
      std::filesystem::path(::testing::SrcDir()) /
      "litert_lm/runtime/testdata/test_lm.cache";

  ASSERT_OK_AND_ASSIGN(auto cache_file, ScopedFile::Open(cache_path));

  auto model_assets = ModelAssets::Create("/path/to/model1.tflite");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets);
  EXPECT_OK(settings);
  settings->SetCacheDir(":nocache");
  // This should be ignored in favor of the explicitly disabled cache dir.
  settings->SetScopedCacheFile(
      std::make_shared<ScopedFile>(std::move(cache_file)));

  EXPECT_THAT(settings->GetWeightCacheFile(), StatusIs(kInvalidArgument));
}

TEST(LlmExecutorConfigTest, GetBackendConfig) {
  auto model_assets = ModelAssets::Create("/path/to/model1");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets, Backend::GPU_ARTISAN);

  (*settings).SetBackendConfig(CreateGpuArtisanConfig());

  auto gpu_config = (*settings).GetBackendConfig<GpuArtisanConfig>();
  EXPECT_OK(gpu_config);
  EXPECT_EQ(gpu_config->num_output_candidates, 1);
  EXPECT_THAT((*settings).GetBackendConfig<CpuConfig>(),
              StatusIs(kInvalidArgument));
}

TEST(LlmExecutorConfigTest, MutableBackendConfig) {
  auto model_assets = ModelAssets::Create("/path/to/model1");
  ASSERT_OK(model_assets);
  auto settings =
      LlmExecutorSettings::CreateDefault(*model_assets, Backend::GPU_ARTISAN);
  (*settings).SetBackendConfig(CreateGpuArtisanConfig());

  auto gpu_config = (*settings).MutableBackendConfig<GpuArtisanConfig>();
  EXPECT_OK(gpu_config);
  gpu_config->num_output_candidates = 2;
  (*settings).SetBackendConfig(gpu_config.value());

  auto gpu_config_after_change =
      (*settings).GetBackendConfig<GpuArtisanConfig>();
  EXPECT_EQ(gpu_config_after_change->num_output_candidates, 2);
  EXPECT_THAT((*settings).MutableBackendConfig<CpuConfig>(),
              StatusIs(kInvalidArgument));
}
}  // namespace
}  // namespace litert::lm
