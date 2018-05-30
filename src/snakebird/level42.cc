#include "main.h"

int main() {
    const char* base_map =
        "..............."
        ".        ##.  ."
        ".        ...  ."
        ".         .   ."
        ".    . >R . * ."
        ".  .   G<     ."
        ".    O#O.  .  ."
        ".   #      .  ."
        ".   .. .   .  ."
        ".   .. .   .  ."
        ".   .. .   .  ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<12, 15, 2, 2, 4, 0>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(42, search(st, map));
    return 0;
}
