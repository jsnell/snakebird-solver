#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".            ."
        ".      *     ."
        ".            ."
        ".      .     ."
        ".  00        ."
        ".  .     .   ."
        ".  ..    .   ."
        ".        ..  ."
        ".  11        ."
        ".  ..        ."
        ".  .. 222..  ."
        ".  ..    ..  ."
        ".  R<<222    ."
        ".  >>G..B<<  ."
        ".  ........  ."
        ".  .......   ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<18, 14, 0, 3, 3, 3>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(90, search(st, map));
    return 0;
}
