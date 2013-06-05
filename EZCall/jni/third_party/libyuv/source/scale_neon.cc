/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/basic_types.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for GCC Neon
#if !defined(YUV_DISABLE_ASM) && defined(__ARM_NEON__)

/**
 * NEON downscalers with interpolation.
 *
 * Provided by Fritz Koenig
 *
 */

void ScaleRowDown2_NEON(const uint8* src_ptr, ptrdiff_t /* src_stride */,
                        uint8* dst, int dst_width) {
  asm volatile (
    "1:                                        \n"
    // load even pixels into q0, odd into q1
    "vld2.u8    {q0,q1}, [%0]!                 \n"
    "vst1.u8    {q0}, [%1]!                    \n"  // store even pixels
    "subs       %2, %2, #16                    \n"  // 16 processed per loop
    "bgt        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst),              // %1
      "+r"(dst_width)         // %2
    :
    : "q0", "q1"              // Clobber List
  );
}

void ScaleRowDown2Int_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst, int dst_width) {
  asm volatile (
    // change the stride to row 2 pointer
    "add        %1, %0                         \n"
    "1:                                        \n"
    "vld1.u8    {q0,q1}, [%0]!                 \n"  // load row 1 and post inc
    "vld1.u8    {q2,q3}, [%1]!                 \n"  // load row 2 and post inc
    "vpaddl.u8  q0, q0                         \n"  // row 1 add adjacent
    "vpaddl.u8  q1, q1                         \n"
    "vpadal.u8  q0, q2                         \n"  // row 2 add adjacent + row1
    "vpadal.u8  q1, q3                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  // downshift, round and pack
    "vrshrn.u16 d1, q1, #2                     \n"
    "vst1.u8    {q0}, [%2]!                    \n"
    "subs       %3, %3, #16                    \n"  // 16 processed per loop
    "bgt        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(src_stride),       // %1
      "+r"(dst),              // %2
      "+r"(dst_width)         // %3
    :
    : "q0", "q1", "q2", "q3"     // Clobber List
   );
}

void ScaleRowDown4_NEON(const uint8* src_ptr, ptrdiff_t /* src_stride */,
                        uint8* dst_ptr, int dst_width) {
  asm volatile (
    "1:                                        \n"
    "vld2.u8    {d0, d1}, [%0]!                \n"
    "vtrn.u8    d1, d0                         \n"
    "vshrn.u16  d0, q0, #8                     \n"
    "vst1.u32   {d0[1]}, [%1]!                 \n"
    "subs       %2, #4                         \n"
    "bgt        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width)         // %2
    :
    : "q0", "q1", "memory", "cc"
  );
}

void ScaleRowDown4Int_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width) {
  asm volatile (
    "add        r4, %0, %3                     \n"
    "add        r5, r4, %3                     \n"
    "add        %3, r5, %3                     \n"
    "1:                                        \n"
    "vld1.u8    {q0}, [%0]!                    \n"   // load up 16x4
    "vld1.u8    {q1}, [r4]!                    \n"
    "vld1.u8    {q2}, [r5]!                    \n"
    "vld1.u8    {q3}, [%3]!                    \n"
    "vpaddl.u8  q0, q0                         \n"
    "vpadal.u8  q0, q1                         \n"
    "vpadal.u8  q0, q2                         \n"
    "vpadal.u8  q0, q3                         \n"
    "vpaddl.u16 q0, q0                         \n"
    "vrshrn.u32 d0, q0, #4                     \n"   // divide by 16 w/rounding
    "vmovn.u16  d0, q0                         \n"
    "vst1.u32   {d0[0]}, [%1]!                 \n"
    "subs       %2, #4                         \n"
    "bgt        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width)         // %2
    : "r"(src_stride)         // %3
    : "r4", "r5", "q0", "q1", "q2", "q3", "memory", "cc"
  );
}

// Down scale from 4 to 3 pixels. Use the neon multilane read/write
// to load up the every 4th pixel into a 4 different registers.
// Point samples 32 pixels to 24 pixels.
void ScaleRowDown34_NEON(const uint8* src_ptr,
                         ptrdiff_t /* src_stride */,
                         uint8* dst_ptr, int dst_width) {
  asm volatile (
    "1:                                        \n"
    "vld4.u8      {d0, d1, d2, d3}, [%0]!      \n" // src line 0
    "vmov         d2, d3                       \n" // order d0, d1, d2
    "vst3.u8      {d0, d1, d2}, [%1]!          \n"
    "subs         %2, #24                      \n"
    "bgt          1b                           \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width)         // %2
    :
    : "d0", "d1", "d2", "d3", "memory", "cc"
  );
}

void ScaleRowDown34_0_Int_NEON(const uint8* src_ptr,
                               ptrdiff_t src_stride,
                               uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vmov.u8      d24, #3                      \n"
    "add          %3, %0                       \n"
    "1:                                        \n"
    "vld4.u8      {d0, d1, d2, d3}, [%0]!      \n" // src line 0
    "vld4.u8      {d4, d5, d6, d7}, [%3]!      \n" // src line 1

    // filter src line 0 with src line 1
    // expand chars to shorts to allow for room
    // when adding lines together
    "vmovl.u8     q8, d4                       \n"
    "vmovl.u8     q9, d5                       \n"
    "vmovl.u8     q10, d6                      \n"
    "vmovl.u8     q11, d7                      \n"

    // 3 * line_0 + line_1
    "vmlal.u8     q8, d0, d24                  \n"
    "vmlal.u8     q9, d1, d24                  \n"
    "vmlal.u8     q10, d2, d24                 \n"
    "vmlal.u8     q11, d3, d24                 \n"

    // (3 * line_0 + line_1) >> 2
    "vqrshrn.u16  d0, q8, #2                   \n"
    "vqrshrn.u16  d1, q9, #2                   \n"
    "vqrshrn.u16  d2, q10, #2                  \n"
    "vqrshrn.u16  d3, q11, #2                  \n"

    // a0 = (src[0] * 3 + s[1] * 1) >> 2
    "vmovl.u8     q8, d1                       \n"
    "vmlal.u8     q8, d0, d24                  \n"
    "vqrshrn.u16  d0, q8, #2                   \n"

    // a1 = (src[1] * 1 + s[2] * 1) >> 1
    "vrhadd.u8    d1, d1, d2                   \n"

    // a2 = (src[2] * 1 + s[3] * 3) >> 2
    "vmovl.u8     q8, d2                       \n"
    "vmlal.u8     q8, d3, d24                  \n"
    "vqrshrn.u16  d2, q8, #2                   \n"

    "vst3.u8      {d0, d1, d2}, [%1]!          \n"

    "subs         %2, #24                      \n"
    "bgt          1b                           \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width),        // %2
      "+r"(src_stride)        // %3
    :
    : "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "d24", "memory", "cc"
  );
}

void ScaleRowDown34_1_Int_NEON(const uint8* src_ptr,
                               ptrdiff_t src_stride,
                               uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vmov.u8      d24, #3                      \n"
    "add          %3, %0                       \n"
    "1:                                        \n"
    "vld4.u8      {d0, d1, d2, d3}, [%0]!      \n" // src line 0
    "vld4.u8      {d4, d5, d6, d7}, [%3]!      \n" // src line 1

    // average src line 0 with src line 1
    "vrhadd.u8    q0, q0, q2                   \n"
    "vrhadd.u8    q1, q1, q3                   \n"

    // a0 = (src[0] * 3 + s[1] * 1) >> 2
    "vmovl.u8     q3, d1                       \n"
    "vmlal.u8     q3, d0, d24                  \n"
    "vqrshrn.u16  d0, q3, #2                   \n"

    // a1 = (src[1] * 1 + s[2] * 1) >> 1
    "vrhadd.u8    d1, d1, d2                   \n"

    // a2 = (src[2] * 1 + s[3] * 3) >> 2
    "vmovl.u8     q3, d2                       \n"
    "vmlal.u8     q3, d3, d24                  \n"
    "vqrshrn.u16  d2, q3, #2                   \n"

    "vst3.u8      {d0, d1, d2}, [%1]!          \n"

    "subs         %2, #24                      \n"
    "bgt          1b                           \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width),        // %2
      "+r"(src_stride)        // %3
    :
    : "r4", "q0", "q1", "q2", "q3", "d24", "memory", "cc"
  );
}

#define HAS_SCALEROWDOWN38_NEON
const uvec8 kShuf38 =
  { 0, 3, 6, 8, 11, 14, 16, 19, 22, 24, 27, 30, 0, 0, 0, 0 };
const uvec8 kShuf38_2 =
  { 0, 8, 16, 2, 10, 17, 4, 12, 18, 6, 14, 19, 0, 0, 0, 0 };
const vec16 kMult38_Div6 =
  { 65536 / 12, 65536 / 12, 65536 / 12, 65536 / 12,
    65536 / 12, 65536 / 12, 65536 / 12, 65536 / 12 };
const vec16 kMult38_Div9 =
  { 65536 / 18, 65536 / 18, 65536 / 18, 65536 / 18,
    65536 / 18, 65536 / 18, 65536 / 18, 65536 / 18 };

// 32 -> 12
void ScaleRowDown38_NEON(const uint8* src_ptr,
                         ptrdiff_t /* src_stride */,
                         uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vld1.u8      {q3}, [%3]                   \n"
    "1:                                        \n"
    "vld1.u8      {d0, d1, d2, d3}, [%0]!      \n"
    "vtbl.u8      d4, {d0, d1, d2, d3}, d6     \n"
    "vtbl.u8      d5, {d0, d1, d2, d3}, d7     \n"
    "vst1.u8      {d4}, [%1]!                  \n"
    "vst1.u32     {d5[0]}, [%1]!               \n"
    "subs         %2, #12                      \n"
    "bgt          1b                           \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width)         // %2
    : "r"(&kShuf38)           // %3
    : "d0", "d1", "d2", "d3", "d4", "d5", "memory", "cc"
  );
}

// 32x3 -> 12x1
void OMITFP ScaleRowDown38_3_Int_NEON(const uint8* src_ptr,
                                      ptrdiff_t src_stride,
                                      uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vld1.u16     {q13}, [%4]                  \n"
    "vld1.u8      {q14}, [%5]                  \n"
    "vld1.u8      {q15}, [%6]                  \n"
    "add          r4, %0, %3, lsl #1           \n"
    "add          %3, %0                       \n"
    "1:                                        \n"

    // d0 = 00 40 01 41 02 42 03 43
    // d1 = 10 50 11 51 12 52 13 53
    // d2 = 20 60 21 61 22 62 23 63
    // d3 = 30 70 31 71 32 72 33 73
    "vld4.u8      {d0, d1, d2, d3}, [%0]!      \n"
    "vld4.u8      {d4, d5, d6, d7}, [%3]!      \n"
    "vld4.u8      {d16, d17, d18, d19}, [r4]!  \n"

    // Shuffle the input data around to get align the data
    //  so adjacent data can be added. 0,1 - 2,3 - 4,5 - 6,7
    // d0 = 00 10 01 11 02 12 03 13
    // d1 = 40 50 41 51 42 52 43 53
    "vtrn.u8      d0, d1                       \n"
    "vtrn.u8      d4, d5                       \n"
    "vtrn.u8      d16, d17                     \n"

    // d2 = 20 30 21 31 22 32 23 33
    // d3 = 60 70 61 71 62 72 63 73
    "vtrn.u8      d2, d3                       \n"
    "vtrn.u8      d6, d7                       \n"
    "vtrn.u8      d18, d19                     \n"

    // d0 = 00+10 01+11 02+12 03+13
    // d2 = 40+50 41+51 42+52 43+53
    "vpaddl.u8    q0, q0                       \n"
    "vpaddl.u8    q2, q2                       \n"
    "vpaddl.u8    q8, q8                       \n"

    // d3 = 60+70 61+71 62+72 63+73
    "vpaddl.u8    d3, d3                       \n"
    "vpaddl.u8    d7, d7                       \n"
    "vpaddl.u8    d19, d19                     \n"

    // combine source lines
    "vadd.u16     q0, q2                       \n"
    "vadd.u16     q0, q8                       \n"
    "vadd.u16     d4, d3, d7                   \n"
    "vadd.u16     d4, d19                      \n"

    // dst_ptr[3] = (s[6 + st * 0] + s[7 + st * 0]
    //             + s[6 + st * 1] + s[7 + st * 1]
    //             + s[6 + st * 2] + s[7 + st * 2]) / 6
    "vqrdmulh.s16 q2, q2, q13                  \n"
    "vmovn.u16    d4, q2                       \n"

    // Shuffle 2,3 reg around so that 2 can be added to the
    //  0,1 reg and 3 can be added to the 4,5 reg. This
    //  requires expanding from u8 to u16 as the 0,1 and 4,5
    //  registers are already expanded. Then do transposes
    //  to get aligned.
    // q2 = xx 20 xx 30 xx 21 xx 31 xx 22 xx 32 xx 23 xx 33
    "vmovl.u8     q1, d2                       \n"
    "vmovl.u8     q3, d6                       \n"
    "vmovl.u8     q9, d18                      \n"

    // combine source lines
    "vadd.u16     q1, q3                       \n"
    "vadd.u16     q1, q9                       \n"

    // d4 = xx 20 xx 30 xx 22 xx 32
    // d5 = xx 21 xx 31 xx 23 xx 33
    "vtrn.u32     d2, d3                       \n"

    // d4 = xx 20 xx 21 xx 22 xx 23
    // d5 = xx 30 xx 31 xx 32 xx 33
    "vtrn.u16     d2, d3                       \n"

    // 0+1+2, 3+4+5
    "vadd.u16     q0, q1                       \n"

    // Need to divide, but can't downshift as the the value
    //  isn't a power of 2. So multiply by 65536 / n
    //  and take the upper 16 bits.
    "vqrdmulh.s16 q0, q0, q15                  \n"

    // Align for table lookup, vtbl requires registers to
    //  be adjacent
    "vmov.u8      d2, d4                       \n"

    "vtbl.u8      d3, {d0, d1, d2}, d28        \n"
    "vtbl.u8      d4, {d0, d1, d2}, d29        \n"

    "vst1.u8      {d3}, [%1]!                  \n"
    "vst1.u32     {d4[0]}, [%1]!               \n"
    "subs         %2, #12                      \n"
    "bgt          1b                           \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width),        // %2
      "+r"(src_stride)        // %3
    : "r"(&kMult38_Div6),     // %4
      "r"(&kShuf38_2),        // %5
      "r"(&kMult38_Div9)      // %6
    : "r4", "q0", "q1", "q2", "q3", "q8", "q9",
      "q13", "q14", "q15", "memory", "cc"
  );
}

// 32x2 -> 12x1
void ScaleRowDown38_2_Int_NEON(const uint8* src_ptr,
                               ptrdiff_t src_stride,
                               uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vld1.u16     {q13}, [%4]                  \n"
    "vld1.u8      {q14}, [%5]                  \n"
    "add          %3, %0                       \n"
    "1:                                        \n"

    // d0 = 00 40 01 41 02 42 03 43
    // d1 = 10 50 11 51 12 52 13 53
    // d2 = 20 60 21 61 22 62 23 63
    // d3 = 30 70 31 71 32 72 33 73
    "vld4.u8      {d0, d1, d2, d3}, [%0]!      \n"
    "vld4.u8      {d4, d5, d6, d7}, [%3]!      \n"

    // Shuffle the input data around to get align the data
    //  so adjacent data can be added. 0,1 - 2,3 - 4,5 - 6,7
    // d0 = 00 10 01 11 02 12 03 13
    // d1 = 40 50 41 51 42 52 43 53
    "vtrn.u8      d0, d1                       \n"
    "vtrn.u8      d4, d5                       \n"

    // d2 = 20 30 21 31 22 32 23 33
    // d3 = 60 70 61 71 62 72 63 73
    "vtrn.u8      d2, d3                       \n"
    "vtrn.u8      d6, d7                       \n"

    // d0 = 00+10 01+11 02+12 03+13
    // d2 = 40+50 41+51 42+52 43+53
    "vpaddl.u8    q0, q0                       \n"
    "vpaddl.u8    q2, q2                       \n"

    // d3 = 60+70 61+71 62+72 63+73
    "vpaddl.u8    d3, d3                       \n"
    "vpaddl.u8    d7, d7                       \n"

    // combine source lines
    "vadd.u16     q0, q2                       \n"
    "vadd.u16     d4, d3, d7                   \n"

    // dst_ptr[3] = (s[6] + s[7] + s[6+st] + s[7+st]) / 4
    "vqrshrn.u16  d4, q2, #2                   \n"

    // Shuffle 2,3 reg around so that 2 can be added to the
    //  0,1 reg and 3 can be added to the 4,5 reg. This
    //  requires expanding from u8 to u16 as the 0,1 and 4,5
    //  registers are already expanded. Then do transposes
    //  to get aligned.
    // q2 = xx 20 xx 30 xx 21 xx 31 xx 22 xx 32 xx 23 xx 33
    "vmovl.u8     q1, d2                       \n"
    "vmovl.u8     q3, d6                       \n"

    // combine source lines
    "vadd.u16     q1, q3                       \n"

    // d4 = xx 20 xx 30 xx 22 xx 32
    // d5 = xx 21 xx 31 xx 23 xx 33
    "vtrn.u32     d2, d3                       \n"

    // d4 = xx 20 xx 21 xx 22 xx 23
    // d5 = xx 30 xx 31 xx 32 xx 33
    "vtrn.u16     d2, d3                       \n"

    // 0+1+2, 3+4+5
    "vadd.u16     q0, q1                       \n"

    // Need to divide, but can't downshift as the the value
    //  isn't a power of 2. So multiply by 65536 / n
    //  and take the upper 16 bits.
    "vqrdmulh.s16 q0, q0, q13                  \n"

    // Align for table lookup, vtbl requires registers to
    //  be adjacent
    "vmov.u8      d2, d4                       \n"

    "vtbl.u8      d3, {d0, d1, d2}, d28        \n"
    "vtbl.u8      d4, {d0, d1, d2}, d29        \n"

    "vst1.u8      {d3}, [%1]!                  \n"
    "vst1.u32     {d4[0]}, [%1]!               \n"
    "subs         %2, #12                      \n"
    "bgt          1b                           \n"
    : "+r"(src_ptr),       // %0
      "+r"(dst_ptr),       // %1
      "+r"(dst_width),     // %2
      "+r"(src_stride)     // %3
    : "r"(&kMult38_Div6),  // %4
      "r"(&kShuf38_2)      // %5
    : "q0", "q1", "q2", "q3", "q13", "q14", "memory", "cc"
  );
}

// 16x2 -> 16x1
void ScaleFilterRows_NEON(uint8* dst_ptr,
                          const uint8* src_ptr, ptrdiff_t src_stride,
                          int dst_width, int source_y_fraction) {
  asm volatile (
    "cmp          %4, #0                       \n"
    "beq          2f                           \n"
    "add          %2, %1                       \n"
    "cmp          %4, #128                     \n"
    "beq          3f                           \n"

    "vdup.8       d5, %4                       \n"
    "rsb          %4, #256                     \n"
    "vdup.8       d4, %4                       \n"
    "1:                                        \n"
    "vld1.u8      {q0}, [%1]!                  \n"
    "vld1.u8      {q1}, [%2]!                  \n"
    "subs         %3, #16                      \n"
    "vmull.u8     q13, d0, d4                  \n"
    "vmull.u8     q14, d1, d4                  \n"
    "vmlal.u8     q13, d2, d5                  \n"
    "vmlal.u8     q14, d3, d5                  \n"
    "vrshrn.u16   d0, q13, #8                  \n"
    "vrshrn.u16   d1, q14, #8                  \n"
    "vst1.u8      {q0}, [%0]!                  \n"
    "bgt          1b                           \n"
    "b            4f                           \n"

    "2:                                        \n"
    "vld1.u8      {q0}, [%1]!                  \n"
    "subs         %3, #16                      \n"
    "vst1.u8      {q0}, [%0]!                  \n"
    "bgt          2b                           \n"
    "b            4f                           \n"

    "3:                                        \n"
    "vld1.u8      {q0}, [%1]!                  \n"
    "vld1.u8      {q1}, [%2]!                  \n"
    "subs         %3, #16                      \n"
    "vrhadd.u8    q0, q1                       \n"
    "vst1.u8      {q0}, [%0]!                  \n"
    "bgt          3b                           \n"
    "4:                                        \n"
    "vst1.u8      {d1[7]}, [%0]                \n"
    : "+r"(dst_ptr),          // %0
      "+r"(src_ptr),          // %1
      "+r"(src_stride),       // %2
      "+r"(dst_width),        // %3
      "+r"(source_y_fraction) // %4
    :
    : "q0", "q1", "d4", "d5", "q13", "q14", "memory", "cc"
  );
}

#endif  // __ARM_NEON__

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

