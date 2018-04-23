int level_09() {
    const char* base_map =
        ".........."        
        ".    *   ."
        ".        ."
        ".        ."
        ".        ."
        ".        ."
        ".        ."
        ".    .~  ."
        ".        ."
        ".        ."
        ".   ~~   ."
        ". .      ."
        ".     .  ."
        ".        ."
        ".        ."
        ".>>R  B< ."
        ".^.....^<."
        "..... ..^."
        "~~~~~~~~~~";

    using St = State<19, 10, 0, 2, 5>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
