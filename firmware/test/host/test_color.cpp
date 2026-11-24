#include "minitest.h"
#include "color.h"

using namespace core;

static RawColor rc(uint16_t r, uint16_t g, uint16_t b, uint16_t c = 0) { RawColor x; x.r = r; x.g = g; x.b = b; x.c = c; return x; }

TEST(color_tcs3200_default_thresholds) {
  Params p; Calib cal;
  CHECK(isRedSample(Sensor::Tcs3200, rc(250, 600, 550), p, cal));     // đỏ
  CHECK(!isRedSample(Sensor::Tcs3200, rc(220, 260, 500), p, cal));    // vàng
  CHECK(!isRedSample(Sensor::Tcs3200, rc(300, 600, 350), p, cal));    // tím
  CHECK(!isRedSample(Sensor::Tcs3200, rc(500, 300, 450), p, cal));    // xanh lá
  CHECK(!isRedSample(Sensor::Tcs3200, rc(120, 130, 110), p, cal));    // nền trắng
  CHECK(!isRedSample(Sensor::Tcs3200, rc(520, 1100, 1000), p, cal));  // đỏ nhưng quá xa (r >= 450)
  CHECK(!isRedSample(Sensor::Tcs3200, rc(0, 0, 0), p, cal));          // không có xung
}

TEST(color_tcs34725_default_thresholds) {
  Params p; Calib cal;
  CHECK(isRedSample(Sensor::Tcs34725, rc(200, 60, 50, 300), p, cal));
  CHECK(!isRedSample(Sensor::Tcs34725, rc(60, 200, 90, 330), p, cal));  // xanh lá
  CHECK(!isRedSample(Sensor::Tcs34725, rc(40, 12, 10, 50), p, cal));    // quá tối (c <= 80)
  CHECK(!isRedSample(Sensor::Tcs34725, rc(150, 140, 60, 340), p, cal)); // vàng: r < 1.10 g
}

TEST(color_chroma_sums_to_one) {
  float c[3];
  chroma(Sensor::Tcs34725, rc(200, 60, 40, 300), c);
  CHECK_NEAR(c[0] + c[1] + c[2], 1.0, 1e-5);
  CHECK_NEAR(c[0], 200.0 / 300, 1e-5);
  chroma(Sensor::Tcs3200, rc(250, 500, 500), c);  // cường độ ~ 1/chu kỳ: đỏ gấp đôi
  CHECK_NEAR(c[0], 0.5, 1e-5);
}

TEST(color_calibrated_rejects_orange_that_defaults_accept) {
  Params p; Calib cal;
  RawColor red = rc(200, 60, 50, 300), orange = rc(230, 160, 50, 360), green = rc(60, 200, 90, 330);
  CHECK(isRedSample(Sensor::Tcs34725, orange, p, cal));  // điểm yếu của ngưỡng mặc định
  calibAdd(cal, Sensor::Tcs34725, kRed, red);
  calibAdd(cal, Sensor::Tcs34725, kYellow, orange);
  calibAdd(cal, Sensor::Tcs34725, kGreen, green);
  CHECK(isRedSample(Sensor::Tcs34725, red, p, cal));
  CHECK(isRedSample(Sensor::Tcs34725, rc(190, 62, 52, 290), p, cal));  // đỏ hơi khác
  CHECK(!isRedSample(Sensor::Tcs34725, orange, p, cal));
  CHECK(!isRedSample(Sensor::Tcs34725, green, p, cal));
}

TEST(color_calib_running_average) {
  Calib cal;
  calibAdd(cal, Sensor::Tcs34725, kRed, rc(200, 50, 50, 300));
  calibAdd(cal, Sensor::Tcs34725, kRed, rc(100, 50, 50, 200));
  // trung bình chroma đỏ của 2 mẫu: (0.6667 + 0.5)/2
  CHECK_NEAR(cal.cent[1][kRed][0], (200.0 / 300 + 100.0 / 200) / 2, 1e-4);
  CHECK(cal.has(Sensor::Tcs34725, kRed));
  CHECK(!cal.has(Sensor::Tcs3200, kRed));
}

TEST(color_vote_majority) {
  ColorVote v;
  for (int i = 0; i < 6; ++i) v.add(i < 4);
  CHECK_EQ(v.reds(), 4); CHECK_EQ(v.total(), 6);
  CHECK(v.decide(4));
  v.reset();
  for (int i = 0; i < 6; ++i) v.add(i < 3);
  CHECK(!v.decide(4));
}
