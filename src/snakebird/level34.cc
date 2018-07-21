#include "main.h"

int main() {
    const char* base_map =
        "..........."
        ".         ."
        ".   ..T.. ."
        ". * .. .. ."
        ".      0  ."
        ".  .   >B ."
        ".   T  G< ."
        ".   ..... ."
        "~~~~~~~~~~~";

    using St = State<Setup<9, 11, 0, 2, 2, 1, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(17, search(st, map));
    return 0;
}
