#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".       .    ."
        ".       .    ."
        ". .....      ."
        ". .   .      ."
        ".   T   .    ."
        ".   .   .    ."
        ".   .G<<T  * ."
        ".   .>>R.    ."
        ".   .....    ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<11, 14, 0, 2, 3, 0, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(16, search(st, map));
    return 0;
}
