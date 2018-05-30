#include "main.h"

int main() {
    const char* base_map =
        "........"
        ".   *  ."
        ". .    ."
        ".O  O. ."
        ".      ."
        ".~.  . ."
        ". >R~~ ."
        ". ^.~  ."
        "~~~~~~~~";

    using St = State<Setup<9, 8, 2, 1, 5>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    EXPECT_EQ(24, search(st, map));
    return 0;
}
