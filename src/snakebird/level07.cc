#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".            ."
        ".       *    ."
        ".            ."
        ".    .       ."
        ".            ."
        ". .          ."
        ". .          ."
        ". .  .   B<< ."
        ".    .  G<<^ ."
        ".       .... ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<12, 14, 0, 2, 4>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(43, search(st, map));
    return 0;
}
