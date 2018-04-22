bool level_03() {
    const char* base_map =
        ".........."
        ".    *   ."
        ".        ."
        ". O    O ."
        ".  ~     ."
        ".  ~......"
        "..... ...."
        "....  ...."
        ". .    ..."
        "~~~~~~~~~~";

    using St = State<10, 10, 2, 1, 5>;
    St::Map map(base_map);
    St st;

    St::Snake blue { 'B', 3, 1 };
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::RIGHT);
    st.add_snake(blue, 0);

    st.print(map);

    return search(st, map);
}
