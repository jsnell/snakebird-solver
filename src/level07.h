bool level_07() {
    const char* base_map =
        ".............."        
        ".       *    ."
        ".            ."
        ".    .       ."
        ".            ."
        ". .          ."
        ". .          ."
        ". .  .       ."
        ".    .       ."
        ".       .... ."
        "~~~~~~~~~~~~~~";

    using St = State<11, 14, 0, 2, 4>;
    St::Map map(base_map);
    St st;

    St::Snake green { 'G', 8, 10 };
    green.grow(St::Snake::LEFT);
    green.grow(St::Snake::LEFT);
    st.add_snake(green, 0);

    St::Snake blue { 'B', 8, 11 };
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::LEFT);
    blue.grow(St::Snake::LEFT);
    st.add_snake(blue, 1);

    st.print(map);

    return search(st, map);
}
