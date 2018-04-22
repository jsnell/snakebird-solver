bool level_05() {
    const char* base_map =
        "........"     
        ".   *  ."
        ". .    ."
        ".O  O. ."
        ".      ."
        ".~.  . ."
        ".   ~~ ."
        ".  .~  ."
        "~~~~~~~~";        

    using St = State<9, 8, 2, 1, 5>;
    St::Map map(base_map);
    St st;

    St::Snake red { 'R', 7, 2 };
    red.grow(St::Snake::UP);
    red.grow(St::Snake::RIGHT);
    st.add_snake(red, 0);

    st.print(map);

    return search(st, map);
}
