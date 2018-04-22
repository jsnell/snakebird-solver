bool level_04() {
    const char* base_map =
        ".........."               
        ".        ."
        ".      * ."
        ".   .    ."
        ".        ."
        "..~   ~. ."
        ". ~      ."
        ". ~~.~   ."
        ".   O    ."
        ".     .  ."
        ".    ..  ."
        ".    ..  ."
        "~~~~~~~~~~";

    using St = State<13, 10, 1, 1, 5>;
    St::Map map(base_map);
    St st;

    St::Snake green { 'G', 3, 3 };
    green.grow(St::Snake::UP);
    green.grow(St::Snake::RIGHT);
    green.grow(St::Snake::RIGHT);
    st.add_snake(green, 0);

    st.print(map);

    return search(st, map);
}
