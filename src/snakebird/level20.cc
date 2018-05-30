#include "main.h"

int main() {
    const char* base_map =
        "................."
        ".               ."
        ".           ..  ."
        ".          .... ."
        ".   >B     .O . ."
        ". * >G    O   . ."
        ".   ..   . .... ."
        ".   ..~      .. ."
        ".   ....  .  .. ."
        ".   ...   .  .. ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<Setup<11, 17, 2, 2, 4>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(50, search(st, map));
    return 0;
}
