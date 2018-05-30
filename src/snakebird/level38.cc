#include "main.h"

int main() {
    const char* base_map =
        "..............."
        ".             ."
        ".             ."
        ".         *   ."
        ".  T          ."
        ".       G<<<  ."
        ".       ..... ."
        ".       T .   ."
        ".       . .   ."
        ".  >>>R ...   ."
        ". .......     ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<12, 15, 0, 2, 4, 0, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(28, search(st, map));
    return 0;
}
