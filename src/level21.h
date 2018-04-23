int level_21() {
    const char* base_map =
        "..........."        
        ".    *    ."
        ".    ~    ."
        ".    .    ."
        ".  O   O  ."
        ".         ."
        ".   >B    ."
        ".~~~^.  ~~."
        "....^O ~~.."
        "... ^<  ..."
        "...     ..."
        "...     ..."
        "~~~~~~~~~~~";

    using St = State<13, 11, 3, 1, 8>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
