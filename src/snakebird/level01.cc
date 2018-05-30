#include "main.h"

int main() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".        ."
        ". .      ."
        ". O  .O. ."
        ".        ."
        ".  .>G   ."
        ".  ....  ."
        ".  ....  ."
        ".  ...   ."
        "~~~~~~~~~~";

    using St = State<Setup<11, 10, 2, 1, 4, 0>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(16, search(st, map));
    return 0;
}
