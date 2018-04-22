bool level_02() {
    const char* base_map =
        "..........."
        "....      ."
        "....      ."
        "....  *   ."
        ". O.      ."
        ".        O."
        ".  .....  ."
        "........  ."
        "........  ."
        "........  ."
        "........  ."
        "~~~~~~~~~~~";

    using St = State<12, 11, 2, 1, 5>;
    St::Map map(base_map);
    St st;

    St::Snake green { 'G', 4, 7 };
    green.grow(St::Snake::DOWN);
    green.grow(St::Snake::LEFT);
    st.add_snake(green, 0);

    st.print(map);

    return search(st, map);
}
