bool level_01() {
    const char* map =
        "........"        
        ".      ."
        ".      ."
        "..     ."
        ".   . .."
        ".      ."
        ". .    ."
        ". .... ."
        ". .... ."
        ". ...  ."
        "~~~~~~~~";

    using St = State<11, 8, 2, 1, 4>;
    St st(map);
    st.add_fruit(4, 1, 0);
    st.add_fruit(4, 5, 1);

    st.set_exit(1, 4);

    St::Snake green { 'G', 6, 3 };
    green.grow(St::Snake::RIGHT);
    st.add_snake(green, 0);

    st.print(map);

    return search(st, map);
}
