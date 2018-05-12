int level_24() {
    const char* base_map =
        ".........."
        ".   O    ."
        ".   .    ."
        ".        ."
        ".        ."
        ".        ."
        ".        ."
        ". 000 v  ."
        ". 000B<* ."
        ".  0 R<  ."
        ".  ...^  ."
        ".  ...   ."
        ".   .    ."
        "~~~~~~~~~~";

    using St = State<Setup<14, 10, 1, 2, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
