bool level_04() {
    const char* map =
        ".........."               
        ".        ."
        ".        ."
        ".   .    ."
        ".        ."
        "..~   ~. ."
        ". ~      ."
        ". ~~.~   ."
        ".        ."
        ".     .  ."
        ".    ..  ."
        ".    ..  ."
        "~~~~~~~~~~";

    using St = State<13, 10, 1, 1, 5>;
    St st(map);
    st.add_fruit(8, 4, 0);

    st.set_exit(2, 7);

    St::Snake green { 'G', 3, 3 };
    green.grow(St::Snake::UP);
    green.grow(St::Snake::RIGHT);
    green.grow(St::Snake::RIGHT);
    st.add_snake(green, 0);

    st.print(map);

    return search(st, map);
}
