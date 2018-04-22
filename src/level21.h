bool level_21() {
    const char* map =
        "..........."        
        ".         ."
        ".    ~    ."
        ".    .    ."
        ".         ."
        ".         ."
        ".         ."
        ".~~~ .  ~~."
        "....   ~~.."
        "...     ..."
        "...     ..."
        "...     ..."
        "~~~~~~~~~~~";

    using St = State<13, 11, 3, 1, 8>;
    St st(map);
    st.add_fruit(3, 3, 0);
    st.add_fruit(3, 7, 1);
    st.add_fruit(8, 5, 2);

    st.set_exit(1, 5);

    St::Snake blue { 'B', 9, 5 };
    blue.grow(St::Snake::LEFT);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::UP);
    blue.grow(St::Snake::RIGHT);
    st.add_snake(blue, 0);

    st.print();

    return search(st);
}
