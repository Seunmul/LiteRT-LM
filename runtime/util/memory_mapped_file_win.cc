// Copyright 2024 The ODML Authors.
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

#include <windows.h>

#include <cstddef>

#include "absl/cleanup/cleanup.h"  // from @com_google_absl
#include "runtime/util/memory_mapped_file.h"
#include "runtime/util/scoped_file.h"
#include "runtime/util/status_macros.h"

namespace litert::lm {
namespace {

class MemoryMappedFileWin : public MemoryMappedFile {
 public:
  MemoryMappedFileWin(HANDLE hmap, uint64_t length, void* data)
      : hmap_(hmap), length_(length), data_(data) {}

  ~MemoryMappedFileWin() override {
    // These checks are now safe for moved-from objects.
    if (data_ != nullptr) {
      ::UnmapViewOfFile(data_);
    }
    if (hmap_ != nullptr) {
      ::CloseHandle(hmap_);
    }
  }

  // Move constructor
  MemoryMappedFileWin(MemoryMappedFileWin&& other) noexcept
      : hmap_(other.hmap_), length_(other.length_), data_(other.data_) {
    // Reset the other object's handles to prevent it from releasing
    // the resources we just took ownership of.
    other.hmap_ = nullptr;
    other.length_ = 0;
    other.data_ = nullptr;
  }

  // Move assignment operator
  MemoryMappedFileWin& operator=(MemoryMappedFileWin&& other) noexcept {
    if (this != &other) {
      // Free our existing resources before taking the new ones.
      if (data_ != nullptr) {
        ::UnmapViewOfFile(data_);
      }
      if (hmap_ != nullptr) {
        ::CloseHandle(hmap_);
      }

      // Transfer ownership from the other object.
      hmap_ = other.hmap_;
      length_ = other.length_;
      data_ = other.data_;

      // Reset the other object.
      other.hmap_ = nullptr;
      other.length_ = 0;
      other.data_ = nullptr;
    }
    return *this;
  }

  // Disable copy operations to enforce single ownership .
  MemoryMappedFileWin(const MemoryMappedFileWin&) = delete;
  MemoryMappedFileWin& operator=(const MemoryMappedFileWin&) = delete;

  uint64_t length() override { return length_; }

  void* data() override { return data_; }

 private:
  HANDLE hmap_;
  uint64_t length_;
  void* data_;
};

absl::StatusOr<std::unique_ptr<MemoryMappedFile>> CreateImpl(HANDLE hfile,
                                                             uint64_t offset,
                                                             uint64_t length,
                                                             const char* key,
                                                             bool writable) {
  RET_CHECK_EQ(offset % MemoryMappedFile::GetOffsetAlignment(), 0)
      << "Offset must be a multiple of allocation granularity: " << offset
      << ", " << MemoryMappedFile::GetOffsetAlignment();

  ASSIGN_OR_RETURN(size_t file_size, ScopedFile::GetSize(hfile));
  RET_CHECK_GE(file_size, length + offset) << "Length and offset too large.";
  if (length == 0) {
    length = file_size - offset;
  }

  DWORD access = FILE_MAP_COPY;
  DWORD protect = PAGE_WRITECOPY;
  if (writable) {
    access = FILE_MAP_ALL_ACCESS;
    protect = PAGE_READWRITE;
  }

  HANDLE hmap = ::OpenFileMappingA(access, false, key);
  if (hmap == NULL) {
    hmap = ::CreateFileMappingA(hfile, nullptr, protect, 0, 0, key);
  }

  RET_CHECK(hmap) << "Failed to create mapping.";
  auto close_hmap = absl::MakeCleanup([hmap] { ::CloseHandle(hmap); });

  ULARGE_INTEGER map_start = {};
  map_start.QuadPart = offset;
  void* mapped_region = ::MapViewOfFile(hmap, access, map_start.HighPart,
                                        map_start.LowPart, length);
  RET_CHECK(mapped_region) << "Failed to map.";

  std::move(close_hmap).Cancel();

  return std::make_unique<MemoryMappedFileWin>(hmap, length, mapped_region);
}

}  // namespace

// static
size_t MemoryMappedFile::GetOffsetAlignment() {
  SYSTEM_INFO sys_info;
  ::GetSystemInfo(&sys_info);
  return sys_info.dwAllocationGranularity;
}

// static
absl::StatusOr<std::unique_ptr<MemoryMappedFile>> MemoryMappedFile::Create(
    absl::string_view path) {
  ASSIGN_OR_RETURN(auto scoped_file, ScopedFile::Open(path));
  return CreateImpl(scoped_file.file(), 0, 0, nullptr, /*writable=*/false);
}

// static
absl::StatusOr<std::unique_ptr<MemoryMappedFile>> MemoryMappedFile::Create(
    HANDLE file, uint64_t offset, uint64_t length, absl::string_view key) {
  return CreateImpl(file, offset, length, key.empty() ? nullptr : key.data(),
                    /*writable=*/false);
}

// static
absl::StatusOr<std::unique_ptr<MemoryMappedFile>>
MemoryMappedFile::CreateMutable(absl::string_view path) {
  ASSIGN_OR_RETURN(auto scoped_file, ScopedFile::OpenWritable(path));
  return CreateImpl(scoped_file.file(), 0, 0, nullptr, /*writable=*/true);
}

absl::StatusOr<std::unique_ptr<MemoryMappedFile>>
MemoryMappedFile::CreateMutable(HANDLE file, uint64_t offset, uint64_t length,
                                absl::string_view key) {
  return CreateImpl(file, offset, length, key.empty() ? nullptr : key.data(),
                    /*writable=*/true);
}

}  // namespace litert::lm
