#include "main.h"

int main() {
    const char* base_map =
        "....................."
        ".                   ."
        ".   ..              ."
        ".   ...             ."
        ".   ...  *          ."
        ".    .              ."
        ".   B<<             ."
        ". # >>R #           ."
        ".   0O0             ."
        ".   000  .          ."
        ".   .#.  .          ."
        ".   ...  .          ."
        "~~~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<13, 21, 1, 2, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(36, search(st, map));
    return 0;
}
