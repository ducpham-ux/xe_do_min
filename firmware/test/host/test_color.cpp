#include "minitest.h"
#include "color.h"

using namespace core;

static RawColor rc(uint16_t r, uint16_t g, uint16_t b) { RawColor x; x.r = r; x.g = g; x.b = b; return x; }

TEST(color_tcs3200_default_thresholds) {
  Params p; Calib cal;
  CHECK(isRedSample(rc(250, 600, 550), p, cal));     // đỏ
  CHECK(!isRedSample(rc(220, 260, 500), p, cal));    // vàng
  CHECK(!isRedSample(rc(300, 600, 350), p, cal));    // tím
  CHECK(!isRedSample(rc(500, 300, 450), p, cal));    // xanh lá
  CHECK(!isRedSample(rc(120, 130, 110), p, cal));    // trắng
  CHECK(!isRedSample(rc(520, 1100, 1000), p, cal));  // đỏ nhưng quá xa (r >= 450)
  CHECK(!isRedSample(rc(0, 0, 0), p, cal));          // không có xung
}

TEST(color_presence_needs_a_bright_channel) {
  Params p;  // presenceMaxUs = 600
  CHECK(colorPresent(rc(250, 600, 550), p));
  CHECK(colorPresent(rc(500, 300, 450), p));
  CHECK(!colorPresent(rc(1500, 1600, 1400), p));   // không có vật: nền tối
  CHECK(!colorPresent(rc(0, 300, 300), p));        // 1 kênh hết thời gian chờ = lỗi đọc
}

TEST(color_chroma_sums_to_one) {
  float c[3];
  chroma(rc(250, 500, 500), c);  // cường độ ~ 1/chu kỳ: đỏ gấp đôi
  CHECK_NEAR(c[0] + c[1] + c[2], 1.0, 1e-5);
  CHECK_NEAR(c[0], 0.5, 1e-5);
}

TEST(color_calibrated_rejects_orange_that_defaults_accept) {
  Params p; Calib cal;
  RawColor red = rc(250, 600, 550), orange = rc(240, 450, 560), green = rc(500, 300, 450);
  CHECK(isRedSample(orange, p, cal));  // điểm yếu của ngưỡng mặc định
  calibAdd(cal, kRed, red);
  calibAdd(cal, kYellow, orange);
  calibAdd(cal, kGreen, green);
  CHECK(isRedSample(red, p, cal));
  CHECK(isRedSample(rc(245, 610, 540), p, cal));  // đỏ hơi khác
  CHECK(!isRedSample(orange, p, cal));
  CHECK(!isRedSample(green, p, cal));
}

TEST(color_calib_running_average) {
  Calib cal;
  calibAdd(cal, kRed, rc(250, 500, 500));   // r chroma = 0.5
  calibAdd(cal, kRed, rc(250, 250, 250));   // r chroma = 1/3
  CHECK_NEAR(cal.cent[kRed][0], (0.5 + 1.0 / 3) / 2, 1e-4);
  CHECK(cal.has(kRed));
  CHECK(!cal.has(kGreen));
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
