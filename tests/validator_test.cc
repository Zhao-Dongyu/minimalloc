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

#include "../src/validator.h"

#include "../src/minimalloc.h"
#include "gtest/gtest.h"

namespace minimalloc {
namespace {

TEST(OverlapsTest, WithOverlap) {
  Buffer bufferA = {.lifespan = {0, 2}};
  Buffer bufferB = {.lifespan = {1, 3}};
  EXPECT_TRUE(Overlaps(bufferA, bufferB));
  EXPECT_TRUE(Overlaps(bufferB, bufferA));
}

TEST(OverlapsTest, WithoutOverlap) {
  Buffer bufferA = {.lifespan = {0, 2}};
  Buffer bufferB = {.lifespan = {3, 5}};
  EXPECT_FALSE(Overlaps(bufferA, bufferB));
  EXPECT_FALSE(Overlaps(bufferB, bufferA));
}

TEST(OverlapsTest, WithoutOverlapEdgeCase) {
  Buffer bufferA = {.lifespan = {0, 2}};
  Buffer bufferB = {.lifespan = {2, 4}};
  EXPECT_FALSE(Overlaps(bufferA, bufferB));
  EXPECT_FALSE(Overlaps(bufferB, bufferA));
}

TEST(OverlapsTest, GapsWithOverlap) {
  Buffer bufferA = {.lifespan = {0, 10}, .gaps = {{.lifespan = {1, 4}},
                                                  {.lifespan = {6, 9}}}};
  Buffer bufferB = {.lifespan = {5, 15}, .gaps = {{.lifespan = {6, 9}},
                                                  {.lifespan = {11, 14}}}};
  EXPECT_TRUE(Overlaps(bufferA, bufferB));
  EXPECT_TRUE(Overlaps(bufferB, bufferA));
}

TEST(OverlapsTest, GapsWithoutOverlap) {
  Buffer bufferA = {.lifespan = {0, 10}, .gaps = {{.lifespan = {1, 9}}}};
  Buffer bufferB = {.lifespan = {5, 15}, .gaps = {{.lifespan = {6, 14}}}};
  EXPECT_FALSE(Overlaps(bufferA, bufferB));
  EXPECT_FALSE(Overlaps(bufferB, bufferA));
}

TEST(OverlapsTest, GapsWithoutOverlapEdgeCaseFirst) {
  Buffer bufferA = {.lifespan = {0, 10}};
  Buffer bufferB = {.lifespan = {5, 15}, .gaps = {{.lifespan = {5, 10}}}};
  EXPECT_FALSE(Overlaps(bufferA, bufferB));
  EXPECT_FALSE(Overlaps(bufferB, bufferA));
}

TEST(OverlapsTest, GapsWithoutOverlapEdgeCaseSecond) {
  Buffer bufferA = {.lifespan = {0, 10}, .gaps = {{.lifespan = {5, 10}}}};
  Buffer bufferB = {.lifespan = {5, 15}};
  EXPECT_FALSE(Overlaps(bufferA, bufferB));
  EXPECT_FALSE(Overlaps(bufferB, bufferA));
}

TEST(ValidatorTest, ValidatesGoodSolution) {
  Problem problem = {
    .buffers = {
        {.lifespan = {0, 1}, .size = 2},
        {.lifespan = {1, 3}, .size = 1},
        {.lifespan = {2, 4}, .size = 1},
        {.lifespan = {3, 5}, .size = 1},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  Solution solution = {.offsets = {0, 0, 1, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, ValidatesGoodSolutionWithGaps) {
  Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {1, 9}}}},
        {.lifespan = {5, 15}, .size = 2, .gaps = {{.lifespan = {6, 14}}}},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  Solution solution = {.offsets = {0, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, ValidatesGoodSolutionWithGapsEdgeCase) {
  Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {1, 8}}}},
        {.lifespan = {5, 15}, .size = 2, .gaps = {{.lifespan = {8, 14}}}},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  Solution solution = {.offsets = {0, 0}};
  EXPECT_EQ(Validate(problem, solution), kGood);
}

TEST(ValidatorTest, InvalidatesBadSolution) {
  Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  Solution solution = {.offsets = {0, 0}};  // wrong # of offsets
  EXPECT_EQ(Validate(problem, solution), kBadSolution);
}

TEST(ValidatorTest, InvalidatesFixedBuffer) {
  Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1, .offset = 0},
      },
      .capacity = 2
  };
  Solution solution = {.offsets = {0, 0, 1}};  // last buffer needs offset @ 0
  EXPECT_EQ(Validate(problem, solution), kBadFixed);
}

TEST(ValidatorTest, InvalidatesNegativeOffset) {
  Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  Solution solution = {.offsets = {0, 0, -1}};  // offset is negative
  EXPECT_EQ(Validate(problem, solution), kBadOffset);
}

TEST(ValidatorTest, InvalidatesOutOfRangeOffset) {
  Problem problem = {
      .buffers = {
          {.lifespan = {0, 1}, .size = 2},
          {.lifespan = {1, 2}, .size = 1},
          {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  Solution solution = {.offsets = {0, 0, 2}};  // buffer lies outside range
  EXPECT_EQ(Validate(problem, solution), kBadOffset);
}

TEST(ValidatorTest, InvalidatesOverlap) {
  Problem problem = {
      .buffers = {
           {.lifespan = {0, 1}, .size = 2},
           {.lifespan = {1, 2}, .size = 1},
           {.lifespan = {1, 2}, .size = 1},
      },
      .capacity = 2
  };
  Solution solution = {.offsets = {0, 0, 0}};  // final two buffers overlap
  EXPECT_EQ(Validate(problem, solution), kBadOverlap);
}

TEST(ValidatorTest, InvalidatesMisalignment) {
  Problem problem = {
      .buffers = {
           {.lifespan = {0, 1}, .size = 2},
           {.lifespan = {1, 2}, .size = 1, .alignment = 2},
      },
      .capacity = 2
  };
  Solution solution = {.offsets = {0, 1}};  // offset of 1 isn't a multiple of 2
  EXPECT_EQ(Validate(problem, solution), kBadAlignment);
}

TEST(ValidatorTest, InvalidatesGapOverlap) {
  Problem problem = {
    .buffers = {
        {.lifespan = {0, 10}, .size = 2, .gaps = {{.lifespan = {1, 7}}}},
        {.lifespan = {5, 15}, .size = 2, .gaps = {{.lifespan = {8, 14}}}},
     },
    .capacity = 2
  };
  // right # of offsets, in range, no overlaps
  Solution solution = {.offsets = {0, 0}};
  EXPECT_EQ(Validate(problem, solution), kBadOverlap);
}

}  // namespace
}  // namespace minimalloc