#include "main.h"

int main() {
    const char* base_map =
        "................"
        ".              ."
        ".   ~          ."
        ".~  .          ."
        ".. *          O."
        ".   B<         ."
        ".  >>G         ."
        ".  ^R<         ."
        ".  ^.^         ."
        ".  ^.^         ."
        ".   .^         ."
        ".   .          ."
        "~~~~~~~~~~~~~~~~";

    using St = State<Setup<13, 16, 1, 3, 7>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    EXPECT_EQ(47, search(st, map));
    return 0;
}
