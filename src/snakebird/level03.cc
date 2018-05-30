#include "main.h"

int main() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".>B      ."
        ".^O    O ."
        ".  ~     ."
        ".  ~......"
        "..... ...."
        "....  ...."
        ". .    ..."
        "~~~~~~~~~~";

    using St = State<Setup<10, 10, 2, 1, 5>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    EXPECT_EQ(27, search(st, map));
    return 0;
}
