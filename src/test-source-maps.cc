/*
 * Copyright 2017 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "source-maps.h"

// Use a macro instead of a function to get meaningful line numbers on failure.
#define EXPECT_SEGMENT_EQ(lhs, rhs) do {           \
    EXPECT_EQ(lhs.generated_col, rhs.generated_col);           \
    EXPECT_EQ(lhs.generated_col_delta, rhs.generated_col_delta);        \
    EXPECT_EQ(lhs.has_source, rhs.has_source);                          \
    EXPECT_EQ(lhs.source, rhs.source);\
    EXPECT_EQ(lhs.source_line, rhs.source_line);\
    EXPECT_EQ(lhs.source_line_delta, rhs.source_line_delta);        \
    EXPECT_EQ(lhs.source_col, rhs.source_col);                    \
    EXPECT_EQ(lhs.source_col_delta, rhs.source_col_delta);      \
    EXPECT_EQ(lhs.has_name, rhs.has_name);                  \
    EXPECT_EQ(lhs.name, rhs.name);                  \
  } while (0)

TEST(source_maps, constructor) { SourceMapGenerator("file", "source-root"); }

TEST(source_maps, sources) {
  SourceMapGenerator smg("source.out", "source-root");
  smg.AddMapping({1, 1}, {2, 3}, "asdf1");
  smg.AddMapping({1, 1}, {2, 2}, "");
  smg.AddMapping({1, 1}, {2, 3}, "asdf2");
  smg.DumpMappings();
  const auto& map = smg.GetMap();
  EXPECT_EQ("source.out", map.file);
  EXPECT_EQ("source-root", map.source_root);
  ASSERT_EQ(3UL, map.sources.size());
  EXPECT_EQ("asdf1", map.sources[0]);
  EXPECT_EQ("", map.sources[1]);
  EXPECT_EQ("asdf2", map.sources[2]);
  SourceMapGenerator smg2("", "");
  const auto& map2 = smg2.GetMap();
  EXPECT_EQ("", map2.file);
  EXPECT_EQ("", map2.source_root);
  EXPECT_EQ(0UL, map2.sources.size());
}

TEST(source_maps, zero_mappings) {
  SourceMapGenerator smg("", "");
  smg.AddMapping({0, 0}, {0, 0}, "");
  smg.DumpMappings();
  const auto& map = smg.GetMap();
  ASSERT_EQ(1UL, map.segment_groups.size());
  EXPECT_EQ(0U, map.segment_groups.back().generated_line);
  ASSERT_EQ(1UL, map.segment_groups.back().segments.size());
  const auto& seg = map.segment_groups.back().segments.back();
  SourceMap::Segment s = {0, 0, true, 0, 0, 0, 0, 0, false, 0};
  EXPECT_SEGMENT_EQ(s, seg);
}

TEST(source_maps, initial_mappings) {
  // Check cases where there is no delta; i.e. the first instances of fields.
  SourceMapGenerator smg("", "");
  smg.AddMapping({4, 1}, {3, 7}, "asdf");
  auto& map = smg.GetMap();
  ASSERT_TRUE(map.Validate(true));
  ASSERT_EQ(4UL, map.segment_groups.size());
  EXPECT_EQ(3U, map.segment_groups.back().generated_line);
  ASSERT_EQ(1UL, map.segment_groups.back().segments.size());
  const auto& seg = map.segment_groups.back().segments.back();
  SourceMap::Segment s = {7, 7, true, 0, 4, 4, 1, 1, false, 0};
  EXPECT_SEGMENT_EQ(s, seg);

  // Duplicate mapping (no new segment)
  smg.AddMapping({4, 1}, {3, 7}, "asdf");
  smg.GetMap();
  ASSERT_TRUE(map.Validate(true));
  ASSERT_EQ(4UL, map.segment_groups.size());
  EXPECT_EQ(3U, map.segment_groups.back().generated_line);
  ASSERT_EQ(1UL, map.segment_groups.back().segments.size());

  // New generated column, same line, same source
  smg.AddMapping({4, 1}, {3, 8}, "asdf");
  smg.GetMap();
  ASSERT_TRUE(map.Validate(true));
  ASSERT_EQ(4UL, map.segment_groups.size());
  EXPECT_EQ(3U, map.segment_groups.back().generated_line);
  ASSERT_EQ(2UL, map.segment_groups.back().segments.size());
  //s = {8, 1, true, 0, 4, 0, 1, 0, false, 0};
  // Not sure which is more readable; pass a whole new segment on one line or
  // update by field name?
  s.generated_col = 8;
  s.generated_col_delta = 1;
  s.source_line_delta = 0;
  s.source_col_delta = 0;
  EXPECT_SEGMENT_EQ(s, map.segment_groups.back().segments.back());

  // New generated column, same line, new source col
  smg.AddMapping({4, 2}, {3, 9}, "asdf");
  smg.GetMap();
  ASSERT_TRUE(map.Validate(true));
  ASSERT_EQ(4UL, map.segment_groups.size());
  EXPECT_EQ(3U, map.segment_groups.back().generated_line);
  ASSERT_EQ(3UL, map.segment_groups.back().segments.size());
  s = {9, 1, true, 0, 4, 0, 2, 1, false, 0};
  EXPECT_SEGMENT_EQ(s, map.segment_groups.back().segments.back());

  // New generated column, same line, new source line
  smg.AddMapping({5, 0}, {3, 10}, "asdf");
  smg.GetMap();
  ASSERT_TRUE(map.Validate(true));
  ASSERT_EQ(4UL, map.segment_groups.size());
  EXPECT_EQ(3U, map.segment_groups.back().generated_line);
  ASSERT_EQ(4UL, map.segment_groups.back().segments.size());
  s = {10, 1, true, 0, 5, 1, 0, 0, false, 0};
  EXPECT_SEGMENT_EQ(s, map.segment_groups.back().segments.back());
}
