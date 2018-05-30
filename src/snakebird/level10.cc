#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".  ...       ."
        ". .... *     ."
        ".    .       ."
        ".  O .   v.. ."
        ".      R<<.  ."
        ".   .... ..  ."
        ".    ... .   ."
        ".      . O   ."
        ".      . ..  ."
        ".      . ..  ."
        ".     .. ..  ."
        ".     ....   ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<14, 14, 2, 1, 6>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(33, search(st, map));
    return 0;
}
