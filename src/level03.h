bool level_03() {
    const char* map =
        ".........."
        ".        ."
        ".        ."
        ".        ."
        ".  ~     ."
        ".  ~......"
        "..... ...."
        "....  ...."
        ". .    ..."
        "~~~~~~~~~~";

    using St = State<10, 10, 2, 1, 5>;
    St st(map);
    st.add_fruit(3, 2, 0);
    st.add_fruit(3, 7, 1);

    st.set_exit(1, 5);

    St::Snake blue { 'B', 3, 1 };
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::RIGHT);
    st.add_snake(blue, 0);

    st.print();

    return search(st);
}
