int level_28() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".        ."
        ".        ."
        ". . 0    ."
        ". ...  # ."
        ".   O.   ."
        ".    .   ."
        ". #.0    ."
        ".    >>B ."
        ".   .G<< ."
        ".  ..... ."
        ".  ..... ."
        "~~~~~~~~~~";

    using St = State<14, 10, 1, 2, 4, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
