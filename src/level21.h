bool level_21() {
    const char* base_map =
        "..........."        
        ".    *    ."
        ".    ~    ."
        ".    .    ."
        ".  O   O  ."
        ".         ."
        ".         ."
        ".~~~ .  ~~."
        ".... O ~~.."
        "...     ..."
        "...     ..."
        "...     ..."
        "~~~~~~~~~~~";

    using St = State<13, 11, 3, 1, 8>;
    St::Map map(base_map);
    St st;

    St::Snake blue { 'B', 9, 5 };
    blue.grow(St::Snake::LEFT);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::RIGHT);
    st.add_snake(blue, 0);

    st.print(map);

    return search(st, map);
}
