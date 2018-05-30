#include "main.h"

int main() {
    const char* base_map =
        "............"
        ".          ."
        ".    ..    ."
        ".    ..    ."
        ".   ..     ."
        ".  ...     ."
        ".   .O .   ."
        ".   .     *."
        ". >B   .   ."
        ". ^..~     ."
        ". ^ ...    ."
        ".          ."
        "~~~~~~~~~~~~";


    using St = State<Setup<13, 12, 1, 1, 5>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    EXPECT_EQ(36, search(st, map));
    return 0;
}
