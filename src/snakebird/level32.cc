#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".            ."
        ".         .  ."
        ".    .    .  ."
        ".  * .   0T  ."
        ".    .B< ... ."
        ".    T>R . . ."
        ". ......##   ."
        ".  .  .      ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<10, 14, 0, 2, 2, 1, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(21, search(st, map));
    return 0;
}
