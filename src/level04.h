int level_04() {
    const char* base_map =
        ".........."               
        ".        ."
        ".  >>G * ."
        ".  ^.    ."
        ".        ."
        "..~   ~. ."
        ". ~      ."
        ". ~~.~   ."
        ".   O    ."
        ".     .  ."
        ".    ..  ."
        ".    ..  ."
        "~~~~~~~~~~";

    using St = State<Setup<13, 10, 1, 1, 5>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    return search(st, map);
}
