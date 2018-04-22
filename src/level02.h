bool level_02() {
    const char* map =
        "..........."
        "....      ."
        "....      ."
        "....      ."
        ".  .      ."
        ".         ."
        ".  .....  ."
        "........  ."
        "........  ."
        "........  ."
        "........  ."
        "~~~~~~~~~~~";

    using St = State<12, 11, 2, 1, 5>;
    St st(map);
    st.add_fruit(4, 2, 0);
    st.add_fruit(5, 9, 1);

    st.set_exit(2, 6);

    St::Snake green { 'G', 5, 8 };
    green.grow(St::Snake::LEFT);
    green.grow(St::Snake::LEFT);
    st.add_snake(green, 0);

    st.print();

    return search(st);
}
