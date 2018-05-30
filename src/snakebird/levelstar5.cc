#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ". .......... ."
        ".  0..... .  ."
        ".  0     *   ."
        ".  0 #   .   ."
        ".  . ..      ."
        ".  .T .      ."
        ".  .  .      ."
        ".  ..#.      ."
        ".    T       ."
        ".            ."
        ".   >>Rv     ."
        ".   ^B<<     ."
        ".   ....     ."
        ".    ..      ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<16, 14, 0, 2, 4, 1, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(69, search(st, map));
    return 0;
}
