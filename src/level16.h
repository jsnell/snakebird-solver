int level_16() {
    const char* base_map =
        ".........."
        ".        ."
        ".      * ."
        ".        ."
        ".        ."
        ".        ."
        ".        ."
        ".  R<< . ."
        ".   .^   ."
        ".      . ."
        ".   .>>G ."
        ".   B<<. ."
        ".   .    ."
        ".      . ."
        ".   .    ."
        "~~~~~~~~~~";

    using St = State<16, 10, 0, 3, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
