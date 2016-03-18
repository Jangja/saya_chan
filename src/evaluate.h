/*
  GodWhale, a  USI shogi(japanese-chess) playing engine derived from NanohaMini
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2010 Marco Costalba, Joona Kiiski, Tord Romstad (Stockfish author)
  Copyright (C) 2014 Kazuyuki Kawabata (NanohaMini author)
  Copyright (C) 2015 ebifrier, espelade, kakiage

  NanohaMini is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  GodWhale is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(EVALUATE_H_INCLUDED)
#define EVALUATE_H_INCLUDED

#include "types.h"
#include "search.h"

class Position;


#ifdef EVAL_TWIG
#include <array>
#include <fstream>
#include <string>
using namespace std;

template <typename Tl, typename Tr>
inline std::array<Tl, 2> operator += (std::array<Tl, 2>& lhs, const std::array<Tr, 2>& rhs) {
  lhs[0] += rhs[0];
  lhs[1] += rhs[1];
  return lhs;
}
template <typename Tl, typename Tr>
inline std::array<Tl, 2> operator -= (std::array<Tl, 2>& lhs, const std::array<Tr, 2>& rhs) {
  lhs[0] -= rhs[0];
  lhs[1] -= rhs[1];
  return lhs;
}


struct Evaluater  {
  // 探索時に参照する評価関数テーブル
  static std::array<int16_t, 2> KPP[nsquare][fe_end][fe_end];
  static std::array<int32_t, 2> KKP[nsquare][nsquare][fe_end];
  static std::array<int32_t, 2> KK[nsquare][nsquare];

  void clear() { memset(this, 0, sizeof(*this)); }

  static void init() {
      if (readSynthesized())
        return;
    }

#define ALL_SYNTHESIZED_EVAL {									\
		FOO(KPP);												\
		FOO(KKP);												\
		FOO(KK);												\
	}
  static bool readSynthesized() {
#define FOO(x) {														\
			std::ifstream ifs((#x "_synthesized.bin"), std::ios::binary); \
			if (ifs) ifs.read(reinterpret_cast<char*>(x), sizeof(x));	\
			else     return false;										\
		}
    ALL_SYNTHESIZED_EVAL
#undef FOO
    return true;
  }
#undef ALL_SYNTHESIZED_EVAL
};


struct EvalSum {
#if defined USE_AVX2_EVAL
  EvalSum(const EvalSum& es) {
    _mm256_store_si256(&mm, es.mm);
  }
  EvalSum& operator = (const EvalSum& rhs) {
    _mm256_store_si256(&mm, rhs.mm);
    return *this;
  }
#elif defined USE_SSE_EVAL
  EvalSum(const EvalSum& es) {
    _mm_store_si128(&m[0], es.m[0]);
    _mm_store_si128(&m[1], es.m[1]);
  }
  EvalSum& operator = (const EvalSum& rhs) {
    _mm_store_si128(&m[0], rhs.m[0]);
    _mm_store_si128(&m[1], rhs.m[1]);
    return *this;
  }
#endif
  EvalSum() {}
  int32_t sum(const Color c) const {
    const int32_t scoreBoard = p[0][0] - p[1][0] + p[2][0];
    const int32_t scoreTurn = p[0][1] + p[1][1] + p[2][1];
    return (c == BLACK ? scoreBoard : -scoreBoard) + scoreTurn;
  }
  EvalSum& operator += (const EvalSum& rhs) {
#if defined USE_AVX2_EVAL
    mm = _mm256_add_epi32(mm, rhs.mm);
#elif defined USE_SSE_EVAL
    m[0] = _mm_add_epi32(m[0], rhs.m[0]);
    m[1] = _mm_add_epi32(m[1], rhs.m[1]);
#else
    p[0][0] += rhs.p[0][0];
    p[0][1] += rhs.p[0][1];
    p[1][0] += rhs.p[1][0];
    p[1][1] += rhs.p[1][1];
    p[2][0] += rhs.p[2][0];
    p[2][1] += rhs.p[2][1];
#endif
    return *this;
  }
  EvalSum& operator -= (const EvalSum& rhs) {
#if defined USE_AVX2_EVAL
    mm = _mm256_sub_epi32(mm, rhs.mm);
#elif defined USE_SSE_EVAL
    m[0] = _mm_sub_epi32(m[0], rhs.m[0]);
    m[1] = _mm_sub_epi32(m[1], rhs.m[1]);
#else
    p[0][0] -= rhs.p[0][0];
    p[0][1] -= rhs.p[0][1];
    p[1][0] -= rhs.p[1][0];
    p[1][1] -= rhs.p[1][1];
    p[2][0] -= rhs.p[2][0];
    p[2][1] -= rhs.p[2][1];
#endif
    return *this;
  }
  EvalSum operator + (const EvalSum& rhs) const { return EvalSum(*this) += rhs; }
  EvalSum operator - (const EvalSum& rhs) const { return EvalSum(*this) -= rhs; }

#if 0
  // ehash 用。
  void encode() {
#if defined USE_AVX2_EVAL
    // EvalSum は atomic にコピーされるので key が合っていればデータも合っている。
#else
    key ^= data[0] ^ data[1] ^ data[2];
#endif
  }
  void decode() { encode(); }
#endif
#if 1
  union {
    std::array<std::array<int32_t, 2>, 3> p;
    struct {
      uint64_t data[3];
      uint64_t key; // ehash用。
    };
#if defined USE_AVX2_EVAL
    __m256i mm;
#endif
#if defined USE_AVX2_EVAL || defined USE_SSE_EVAL
    __m128i m[2];
#endif
  };
#endif
};
#endif
#endif // !defined(EVALUATE_H_INCLUDED)
