#include "main.h"

int main() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".        ."
        ".        ."
        ". . 0    ."
        ". ...  # ."
        ".   O.   ."
        ".    .   ."
        ". #.0    ."
        ".    >>B ."
        ".   .G<< ."
        ".  ..... ."
        ".  ..... ."
        "~~~~~~~~~~";

    using St = State<Setup<14, 10, 1, 2, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(49, search(st, map));
    return 0;
}
