#include "main.h"

int main() {
    const char* base_map =
        "............."
        ".     *     ."
        ".           ."
        ".           ."
        ".           ."
        ".           ."
        ".  >>R      ."
        ".   ..   00 ."
        ".   .. . 00 ."
        ".   ..   .. ."
        ".   ....... ."
        ".   ....... ."
        "~~~~~~~~~~~~~";

    using St = State<Setup<13, 13, 0, 1, 3, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(45, search(st, map));
    return 0;
}
