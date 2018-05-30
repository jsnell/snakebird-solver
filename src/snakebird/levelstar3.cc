#include "main.h"

int main() {
    const char* base_map =
        "...................."
        ".                  ."
        ".           .      ."
        ".           .      ."
        ".           .      ."
        ".           .      ."
        ". *        000  #  ."
        ".          0R0     ."
        ".          0^0     ."
        ".           ^      ."
        ".         >B^   #  ."
        ".     .   ^>>G.##  ."
        ".    ..    #.##    ."
        ".                  ."
        ".    ..            ."
        ".        .         ."
        "~~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<17, 20, 0, 3, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(63, search(st, map));
    return 0;
}
