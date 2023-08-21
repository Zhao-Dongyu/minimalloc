/*
Copyright 2023 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef MINIMALLOC_H_
#define MINIMALLOC_H_

#include <stdint.h>

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "absl/status/statusor.h"

// MiniMalloc is a lightweight memory allocator for hardware-accelerated ML.
namespace minimalloc {

template<typename T>
struct Interval {
  T lower_ = 0;
  T upper_ = 0;

  T lower() const { return lower_; }
  T upper() const { return upper_; }

  bool operator==(const Interval<T>& other) const {
    return lower_ == other.lower_ && upper_ == other.upper_;
  }
  bool operator<(const Interval<T>& other) const {
    if (lower_ != other.lower_) return lower_ < other.lower_;
    return upper_ < other.upper_;
  }
};

using BufferIdx = int64_t;  // An index into a Problem's list of buffers.
using Capacity = int64_t;  // A maximum memory capacity defined @ Problem level.
using Offset = int64_t;  // A memory address (eg in bytes) assigned to a buffer.
using TimeValue = int64_t;  // An abstract unitless start/end time of a buffer.
using Area = int64_t;  // The unitless product of a buffer's length and size.
using Lifespan = Interval<TimeValue>;

struct Gap {
  Lifespan lifespan;  // The interval where this gap applies.
  bool operator==(const Gap& x) const;
};

struct Buffer {
  std::string id;  // A unique identifier for this buffer (used in file I/O).
  Lifespan lifespan = {0, 0};  // Half-open.
  int64_t size = 0;  // The amount of memory allocated during the lifespan.
  int64_t alignment = 1;  // The lowest common denominator of assigned offsets.
  std::vector<Gap> gaps;  // Slots where this buffer is inactive.
  std::optional<Offset> offset;  // If present, the fixed pos. of this buffer.
  Area area() const;  // The product of this buffer's size and lifespan length.
  bool operator==(const Buffer& x) const;
};

struct Solution {
  std::vector<Offset> offsets;
  bool operator==(const Solution& x) const;
};

struct Problem {
  std::vector<Buffer> buffers;

  // The total size of the memory address space (i.e. the maximum amount of
  // available memory that all buffers must be packed within).  No buffer may be
  // assigned an offset such that offset + size > capacity.
  Capacity capacity = 0;

  // Extracts a solution from the offset value of each buffer, which is cleared.
  absl::StatusOr<Solution> strip_solution();

  bool operator==(const Problem& x) const;
};

}  // namespace minimalloc

#endif  // MINIMALLOC_H_