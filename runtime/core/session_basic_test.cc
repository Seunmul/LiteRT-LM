#include "runtime/core/session_basic.h"

#include <filesystem>  // NOLINT: Required for path manipulation.
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/time/time.h"  // from @com_google_absl
#include "runtime/components/sentencepiece_tokenizer.h"
#include "runtime/components/tokenizer.h"
#include "runtime/engine/engine_settings.h"
#include "runtime/engine/io_types.h"
#include "runtime/executor/executor_settings_base.h"
#include "runtime/executor/fake_llm_executor.h"
#include "runtime/executor/llm_executor.h"
#include "runtime/framework/thread_options.h"
#include "runtime/framework/threadpool.h"
#include "runtime/util/test_utils.h"  // NOLINT

namespace litert::lm {
namespace {

constexpr char kTestdataDir[] =
    "litert_lm/runtime/components/testdata/";

class SessionBasicTest : public testing::Test {
 protected:
  void SetUp() override {
    auto tokenizer = SentencePieceTokenizer::CreateFromFile(
        (std::filesystem::path(::testing::SrcDir()) / kTestdataDir /
         "sentencepiece.model")
            .string());
    ASSERT_OK(tokenizer);
    tokenizer_ = std::move(*tokenizer);
    // The prefill tokens are the expected tokens that will be passed in at each
    // time the Prefill function is called. The values are the token ids of the
    // input prompt "Hello World!".
    std::vector<std::vector<int>> prefill_tokens = {
        {2, 90, 547, 58, 735, 210, 466, 2294}};
    // The decode tokens are the expected tokens that will be returned by the
    // Decode function. The values are the token ids of the output response
    // "How's it going?" followed by the stop token id (2294).
    std::vector<std::vector<int>> decode_tokens = {{224}, {24}, {8},    {66},
                                                   {246}, {18}, {2295}, {2294}};
    executor_ =
        std::make_unique<FakeLlmExecutor>(2560, prefill_tokens, decode_tokens);

    sampler_params_.set_type(proto::SamplerParameters::TYPE_UNSPECIFIED);

    // Creating the thread pool of a single thread to execute the works.
    worker_thread_pool_ = std::make_unique<ThreadPool>(/*name_prefix=*/"engine",
                                                       /*max_num_threads=*/1);
  }

  std::unique_ptr<Tokenizer> tokenizer_;
  std::unique_ptr<LlmExecutor> executor_;
  proto::SamplerParameters sampler_params_;
  std::unique_ptr<ThreadPool> worker_thread_pool_;
};

TEST_F(SessionBasicTest, RunPrefill) {
  const std::vector<std::vector<int>> stop_token_ids = {{2294}};
  SessionConfig session_config = SessionConfig::CreateDefault();
  session_config.GetMutableSamplerParams() = sampler_params_;
  session_config.GetMutableStopTokenIds() = stop_token_ids;
  session_config.SetStartTokenId(2);
  session_config.SetSamplerBackend(Backend::CPU);
  auto session = SessionBasic::Create(
      executor_.get(), tokenizer_.get(), session_config,
      /*benchmark_info=*/std::nullopt, worker_thread_pool_.get());
  EXPECT_OK((*session)->RunPrefill({InputText("Hello World!")}));
}

TEST_F(SessionBasicTest, RunDecode) {
  const std::vector<std::vector<int>> stop_token_ids = {{2294}};
  SessionConfig session_config = SessionConfig::CreateDefault();
  session_config.GetMutableSamplerParams() = sampler_params_;
  session_config.GetMutableStopTokenIds() = stop_token_ids;
  session_config.SetStartTokenId(2);
  session_config.SetSamplerBackend(Backend::CPU);
  auto session =
      SessionBasic::Create(executor_.get(), tokenizer_.get(), session_config,
                           std::nullopt, worker_thread_pool_.get());
  EXPECT_OK((*session)->RunPrefill({InputText("Hello World!")}));
  auto responses = (*session)->RunDecode();
  EXPECT_OK(responses);
  EXPECT_EQ(responses->GetNumOutputCandidates(), 1);
  EXPECT_EQ(*(responses->GetResponseTextAt(0)), " How's it going?!");
}

class TestObserver : public InferenceObservable {
 public:
  void OnDone() override { done_ = true; }

  bool IsDone() { return done_; }

 private:
  bool done_ = false;
};

TEST_F(SessionBasicTest, RunPrefillAsync) {
  const std::vector<std::vector<int>> stop_token_ids = {{2294}};
  SessionConfig session_config = SessionConfig::CreateDefault();
  session_config.GetMutableSamplerParams() = sampler_params_;
  session_config.SetStartTokenId(2);
  session_config.GetMutableStopTokenIds() = stop_token_ids;
  session_config.SetSamplerBackend(Backend::CPU);
  auto session =
      SessionBasic::Create(executor_.get(), tokenizer_.get(), session_config,
                           std::nullopt, worker_thread_pool_.get());
  TestObserver observer;
  EXPECT_OK(
      (*session)->RunPrefillAsync({InputText("Hello World!")}, &observer));
  // Wait for the async call to finish.
  EXPECT_OK(worker_thread_pool_->WaitUntilDone(absl::Seconds(100)));
  EXPECT_TRUE(observer.IsDone());
}

TEST_F(SessionBasicTest, RunDecodeAsync) {
  const std::vector<std::vector<int>> stop_token_ids = {{2294}};
  SessionConfig session_config = SessionConfig::CreateDefault();
  session_config.GetMutableSamplerParams() = sampler_params_;
  session_config.GetMutableStopTokenIds() = stop_token_ids;
  session_config.SetSamplerBackend(Backend::CPU);
  auto session =
      SessionBasic::Create(executor_.get(), tokenizer_.get(), session_config,
                           std::nullopt, worker_thread_pool_.get());
  TestObserver observer;
  EXPECT_OK(
      (*session)->RunPrefillAsync({InputText("Hello World!")}, &observer));
  EXPECT_OK((*session)->RunDecodeAsync(&observer));
  EXPECT_OK(worker_thread_pool_->WaitUntilDone(absl::Seconds(100)));
  EXPECT_TRUE(observer.IsDone());
}

}  // namespace
}  // namespace litert::lm
