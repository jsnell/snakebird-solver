#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".            ."
        ".      0     ."
        ".     #1     ."
        ".      1     ."
        ".    . .   * ."
        ".    #       ."
        ".    #       ."
        ". >>R  #B<<  ."
        ". ^.##O ..   ."
        ".  ..   ..   ."
        ".  .     .   ."
        ".     O      ."
        ".            ."
        ".            ."
        ".     .      ."
        ".    #.#     ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<18, 14, 2, 2, 6, 2>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(77, search(st, map));
    return 0;
}
