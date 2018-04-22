bool level_05() {
    const char* map =
        "........"     
        ".      ."
        ". .    ."
        ".    . ."
        ".      ."
        ".~.  . ."
        ".   ~~ ."
        ".  .~  ."
        "~~~~~~~~";        

    using St = State<9, 8, 2, 1, 5>;
    St st(map);
    st.add_fruit(3, 1, 0);
    st.add_fruit(3, 4, 1);

    st.set_exit(1, 4);

    St::Snake red { 'R', 7, 2 };
    red.grow(St::Snake::UP);
    red.grow(St::Snake::RIGHT);
    st.add_snake(red, 0);

    st.print(map);

    return search(st, map);
}
