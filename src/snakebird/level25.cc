#include "main.h"

int main() {
    const char* base_map =
        "..............."
        ".      *      ."
        ".             ."
        ".      .      ."
        ".             ."
        ".             ."
        ".             ."
        ".             ."
        ".      0      ."
        ".     ...     ."
        ".    0   0    ."
        ".             ."
        ".   >G 0 B<   ."
        ".   ^.   .^   ."
        ".   ^.   .    ."
        ".    .   .    ."
        ".    .   .    ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<18, 15, 0, 2, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(35, search(st, map));
    return 0;
}
