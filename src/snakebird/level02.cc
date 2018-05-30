#include "main.h"

int main() {
    const char* base_map =
        "..........."
        "....      ."
        "....  *   ."
        "....      ."
        ". O.   v  ."
        ".     G< O."
        ".  .....  ."
        "........  ."
        "........  ."
        "........  ."
        "........  ."
        "~~~~~~~~~~~";

    using St = State<Setup<12, 11, 2, 1, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(25, search(st, map));
    return 0;
}
