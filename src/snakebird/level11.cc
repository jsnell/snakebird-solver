#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".       ~~   ."
        ".    ~~~.~~  ."
        ".   ~......~ ."
        ".   ..  .... ."
        ".   .. O O . ."
        ".    .O.O O. ."
        ". * >G O  .. ."
        ".   ^... O.. ."
        ".   ^<<....  ."
        ".     ^ ..   ."
        ".     ^      ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<13, 14, 7, 1, 15>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(35, search(st, map));
    return 0;
}
