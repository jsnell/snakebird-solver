#include "main.h"

int main() {
    const char* base_map =
        "......................"
        ".                    ."
        ".           *        ."
        ".                    ."
        ".                    ."
        ".    .               ."
        ".     00             ."
        ".    11              ."
        ".    .               ."
        ".    >v              ."
        ".    .>>B            ."
        ".   G<<<<            ."
        ".    .               ."
        ".    .               ."
        ".    .               ."
        "~~~~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<16, 22, 0, 2, 5, 2>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(51, search(st, map));
    return 0;
}
