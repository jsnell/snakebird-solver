#include "main.h"

int main() {
    const char* base_map =
        "................"
        ".          ... ."
        ".          ....."
        ".          #...."
        ".          #   ."
        ".    O     # * ."
        ". B<           ."
        ".  R       #...."
        ".  ^       #...."
        ".  ^0      #...."
        ".  .########.. ."
        ".  ...  ........"
        "~~~~~~~~~~~~~~~~";

    using St = State<Setup<13, 16, 1, 2, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(35, search(st, map));
    return 0;
}
