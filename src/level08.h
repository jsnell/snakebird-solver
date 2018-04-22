bool level_08() {
    const char* base_map =
        ".................."        
        ".              * ."
        ".                ."
        ".             ...."
        ".           ~~~..."
        ".......~~~  ~    ."
        ". .....  ~~.~    ."
        "~~~~~~~~~~~~~~~~~~";

    using St = State<8, 18, 0, 2, 5>;
    St::Map map(base_map);
    St st;

    St::Snake red { 'R', 4, 2 };
    red.grow(St::Snake::RIGHT);
    red.grow(St::Snake::UP);
    red.grow(St::Snake::RIGHT);
    red.grow(St::Snake::RIGHT);
    st.add_snake(red, 0);

    St::Snake green { 'G', 4, 6 };
    green.grow(St::Snake::LEFT);
    green.grow(St::Snake::LEFT);
    st.add_snake(green, 1);

    st.print(map);

    return search(st, map);
}
