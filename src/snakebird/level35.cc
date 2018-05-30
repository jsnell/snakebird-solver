#include "main.h"

int main() {
    const char* base_map =
        "..........."
        ". ....... ."
        ".  .....  ."
        ".  .....  ."
        ".         ."
        ".      .  ."
        ".      #  ."
        ".       O ."
        ".    T .  ."
        ".      .  ."
        ".         ."
        ". * >>B   ."
        ".   ^.    ."
        ".    .    ."
        ".     T   ."
        ".    ##   ."
        ".    O    ."
        ".    ..   ."
        ".    ..   ."
        ".    ..   ."
        "~~~~~~~~~~~";

    using St = State<Setup<21, 11, 2, 1, 6, 0, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(29, search(st, map));
    return 0;
}
