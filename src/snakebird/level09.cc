#include "main.h"

int main() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".        ."
        ".        ."
        ".        ."
        ".        ."
        ".        ."
        ".    .~  ."
        ".        ."
        ".        ."
        ".   ~~   ."
        ". .      ."
        ".     .  ."
        ".        ."
        ".        ."
        ".>>R  B< ."
        ".^.....^<."
        "..... ..^."
        "~~~~~~~~~~";

    using St = State<Setup<19, 10, 0, 2, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(37, search(st, map));
    return 0;
}
