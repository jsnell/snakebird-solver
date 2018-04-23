int level_03() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".>B      ."
        ".^O    O ."
        ".  ~     ."
        ".  ~......"
        "..... ...."
        "....  ...."
        ". .    ..."
        "~~~~~~~~~~";

    using St = State<10, 10, 2, 1, 5>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    return search(st, map);
}
