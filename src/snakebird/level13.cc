#include "main.h"

int main() {
    const char* base_map =
        "................."
        ".               ."
        ".               ."
        ".     ~~~~      ."
        ".     ...~      ."
        ".     ...~   .  ."
        ".            .  ."
        ".   *       B<< ."
        ".            .^ ."
        ".            .  ."
        ". ...       G<< ."
        ".  ...       .^ ."
        ".            .^ ."
        ".            .  ."
        ".            .  ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<Setup<16, 17, 0, 2, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(44, search(st, map));
    return 0;
}
