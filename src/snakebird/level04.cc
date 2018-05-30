#include "main.h"

int main() {
    const char* base_map =
        ".........."
        ".        ."
        ".  >>G * ."
        ".  ^.    ."
        ".        ."
        "..~   ~. ."
        ". ~      ."
        ". ~~.~   ."
        ".   O    ."
        ".     .  ."
        ".    ..  ."
        ".    ..  ."
        "~~~~~~~~~~";

    using St = State<Setup<13, 10, 1, 1, 5>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    EXPECT_EQ(30, search(st, map));
    return 0;
}
