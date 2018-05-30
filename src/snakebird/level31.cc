#include "main.h"

int main() {
    const char* base_map =
        "................."
        ".               ."
        ".      ....     ."
        ".       ..      ."
        ".               ."
        ".          T    ."
        ".          .    ."
        ".        G .    ."
        ". *   T  ^      ."
        ".     .  ^<     ."
        ".     ......    ."
        ".       ..      ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<Setup<13, 17, 0, 1, 4, 0, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(8, search(st, map));
    return 0;
}
