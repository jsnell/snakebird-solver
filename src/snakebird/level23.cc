#include "main.h"

int main() {
    const char* base_map =
        "..................."
        ".                 ."
        ".                 ."
        ".          #      ."
        ".          ###### ."
        ".        0 .....# ."
        ".        111 ...# ."
        ". >>>>B    ...... ."
        ". ^...... ....... ."
        ". ^......0....... ."
        ". ^  .*#.     .   ."
        ". ^  . #.  .  .   ."
        ". ^  . #      .   ."
        ". ^  . #      .   ."
        ".    . # #    .   ."
        ".    .   .    .   ."
        "~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<17, 19, 0, 1, 11, 2>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(53, search(st, map));
    return 0;
}
