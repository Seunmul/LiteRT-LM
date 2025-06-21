#include "runtime/components/huggingface_tokenizer.h"

#include <fcntl.h>

#include <filesystem>  // NOLINT: Required for path manipulation.
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"  // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "absl/strings/str_cat.h"  // from @com_google_absl
#include "absl/strings/string_view.h"  // from @com_google_absl
#include "runtime/util/status_macros.h"  // NOLINT
#include "runtime/util/test_utils.h"     // NOLINT

namespace litert::lm {
namespace {

constexpr char kTestdataDir[] =
    "litert_lm/runtime/components/testdata/";

std::string GetHuggingFaceModelPath() {
  return (std::filesystem::path(::testing::SrcDir()) / kTestdataDir /
          "tokenizer.json")
      .string();
}

absl::StatusOr<std::string> GetContents(const std::string& path) {
  std::ifstream input_stream(path);
  if (!input_stream.is_open()) {
    return absl::InternalError(absl::StrCat("Could not open file: ", path));
  }

  std::string content;
  content.assign((std::istreambuf_iterator<char>(input_stream)),
                 (std::istreambuf_iterator<char>()));
  return std::move(content);
}

TEST(HuggingFaceTokenizerTtest, CreateFromFile) {
  auto tokenizer_or =
      HuggingFaceTokenizer::CreateFromFile(GetHuggingFaceModelPath());
  EXPECT_TRUE(tokenizer_or.ok());
}

TEST(HuggingFaceTokenizerTtest, CreateFromBuffer) {
  ASSERT_OK_AND_ASSIGN(auto json, GetContents(GetHuggingFaceModelPath()));
  auto tokenizer_or = HuggingFaceTokenizer::CreateFromJson(json);
  EXPECT_TRUE(tokenizer_or.ok());
}

TEST(HuggingFaceTokenizerTtest, Create) {
  auto tokenizer_or =
      HuggingFaceTokenizer::CreateFromFile(GetHuggingFaceModelPath());
  EXPECT_TRUE(tokenizer_or.ok());
}

TEST(HuggingFaceTokenizerTest, TextToTokenIds) {
  auto tokenizer_or =
      HuggingFaceTokenizer::CreateFromFile(GetHuggingFaceModelPath());
  ASSERT_OK(tokenizer_or);
  auto tokenizer = std::move(tokenizer_or.value());

  absl::string_view text = "How's it going?";
  auto ids_or = tokenizer->TextToTokenIds(text);
  ASSERT_OK(ids_or);

  EXPECT_THAT(ids_or.value(), ::testing::ElementsAre(2020, 506, 357, 2045, 47));
}

TEST(HuggingFaceTokenizerTest, TokenIdsToText) {
  auto tokenizer_or =
      HuggingFaceTokenizer::CreateFromFile(GetHuggingFaceModelPath());
  ASSERT_OK(tokenizer_or);
  auto tokenizer = std::move(tokenizer_or.value());

  const std::vector<int> ids = {2020, 506, 357, 2045, 47};
  auto text_or = tokenizer->TokenIdsToText(ids);
  ASSERT_OK(text_or);

  EXPECT_EQ(text_or.value(), "How's it going?");
}

}  // namespace
}  // namespace litert::lm
