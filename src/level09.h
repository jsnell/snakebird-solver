bool level_09() {
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
        ".        ."
        ". .....  ."
        "..... .. ."
        "~~~~~~~~~~";

    using St = State<19, 10, 0, 2, 5>;
    St::Map map(base_map);
    St st;

    St::Snake red { 'R', 16, 1 };
    red.grow(St::Snake::UP);
    red.grow(St::Snake::RIGHT);
    red.grow(St::Snake::RIGHT);
    st.add_snake(red, 0);

    St::Snake blue { 'B', 17, 8 };
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::LEFT);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::LEFT);
    st.add_snake(blue, 1);

    st.print(map);

    return search(st, map);
}
