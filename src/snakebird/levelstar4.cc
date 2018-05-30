#include "main.h"

int main() {
    const char* base_map =
        "............."
        ".     *     ."
        ".           ."
        ".           ."
        ".           ."
        ".     #     ."
        ".   0   0   ."
        ".   1  #1   ."
        ".   2   2   ."
        ".  G<<#>>R  ."
        ". ......... ."
        ".   .....   ."
        ".    ..     ."
        "~~~~~~~~~~~~~";

    using St = State<Setup<14, 13, 0, 2, 3, 3>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(44, search(st, map));
    return 0;
}
