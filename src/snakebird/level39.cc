#include "main.h"

int main() {
    const char* base_map =
        "................"
        ".        ..... ."
        ".         .... ."
        ".         .... ."
        ".              ."
        ".          *   ."
        ".              ."
        ".              ."
        ".        ....  ."
        ".        ....  ."
        ".         ...  ."
        ".        ....  ."
        ".  >>G   ..... ."
        ".  ^00    .... ."
        ".  ^11   ..... ."
        ". ....  ...... ."
        ". ...... ..... ."
        ".  ....   . .  ."
        ".  ...    . .  ."
        "~~~~~~~~~~~~~~~~";

    using St = State<Setup<20, 16, 0, 1, 5, 2>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(53, search(st, map));
    return 0;
}
